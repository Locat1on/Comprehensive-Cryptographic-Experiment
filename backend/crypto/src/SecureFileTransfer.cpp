#include "SecureFileTransfer.h"
#include "DHProtocol.h"
#include "RSA.h"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iomanip>
#include <mutex>
#include <random>
#include <sstream>
#include <stdexcept>
#include <unordered_map>
#ifdef _WIN32
#include <direct.h>
#else
#include <sys/stat.h>
#endif

namespace {

struct TransferSession {
    std::string id;
    std::string fileName;
    std::string cipher;
    std::string path;
    std::string sharedKey;
    std::string manifest;
    std::string finalHash;
    std::string signature;
    uint64_t fileSize = 0;
    size_t chunkSize = 4 * 1024 * 1024;
    int totalChunks = 0;
    int receivedChunks = 0;
    std::string status = "pending";
};

static std::mutex g_mutex;
static std::unordered_map<std::string, TransferSession> g_sessions;
static const char* STORE_DIR = "transfers";

static uint64_t modPow(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1 % mod;
    base %= mod;
    while (exp > 0) {
        if (exp & 1ULL) result = static_cast<uint64_t>((unsigned __int128)result * base % mod);
        base = static_cast<uint64_t>((unsigned __int128)base * base % mod);
        exp >>= 1U;
    }
    return result;
}

static std::string makeId() {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;
    std::ostringstream oss;
    oss << "tx_" << std::hex << now << "_" << dist(gen);
    return oss.str();
}

static int hexValue(char c) {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
}

static std::string hexToBytes(const std::string& hex) {
    std::string out;
    out.reserve(hex.size() / 2);
    for (size_t i = 0; i + 1 < hex.size(); i += 2)
        out.push_back(static_cast<char>((hexValue(hex[i]) << 4) | hexValue(hex[i + 1])));
    return out;
}

static std::string xorCrypt(const std::string& data,
                            const std::string& key,
                            const std::string& id,
                            int chunkIndex) {
    std::string out = data;
    size_t pos = 0;
    uint64_t counter = 0;
    while (pos < out.size()) {
        std::string seed = key + "|" + id + "|" + std::to_string(chunkIndex) + "|" + std::to_string(counter++);
        std::string stream = hexToBytes(Hash::compute(seed, Hash::SHA1));
        for (size_t i = 0; i < stream.size() && pos < out.size(); ++i, ++pos)
            out[pos] = static_cast<char>(static_cast<unsigned char>(out[pos]) ^ static_cast<unsigned char>(stream[i]));
    }
    return out;
}

static RSA::KeyPair signingKey() {
    return RSA::generate(61, 53, 17);
}

static std::string signDigest(const std::string& digestHex) {
    RSA rsa(signingKey());
    std::string h = digestHex.size() > 32 ? digestHex.substr(digestHex.size() - 32) : digestHex;
    return rsa.sign(BigInt128::fromHex(h)).toHex();
}

static TransferSession& requireSession(const std::string& id) {
    auto it = g_sessions.find(id);
    if (it == g_sessions.end()) throw std::runtime_error("SecureFileTransfer: unknown transfer id");
    return it->second;
}

static void ensureStore() {
#ifdef _WIN32
    _mkdir(STORE_DIR);
#else
    mkdir(STORE_DIR, 0755);
#endif
}

} // namespace

SecureFileTransfer::UploadResult SecureFileTransfer::receiveAndEncrypt(
    const std::string& fileData,
    const std::string& cipher,
    const Config& cfg)
{
    auto start = startUpload("upload.bin", static_cast<uint64_t>(fileData.size()), cipher, cfg);
    receiveChunk(start.id, 0, fileData);
    return finishUpload(start.id);
}

SecureFileTransfer::StartResult SecureFileTransfer::startUpload(
    const std::string& fileName,
    uint64_t fileSize,
    const std::string& cipher,
    const Config& cfg)
{
    if (cfg.chunkSize == 0) throw std::runtime_error("SecureFileTransfer: chunk size must be positive");
    ensureStore();

    const uint64_t p = 65537;
    const uint64_t g = 3;
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(2, p - 2);
    uint64_t clientPriv = dist(gen);
    uint64_t serverPriv = dist(gen);
    uint64_t clientPub = modPow(g, clientPriv, p);
    uint64_t serverPub = modPow(g, serverPriv, p);
    uint64_t shared = modPow(clientPub, serverPriv, p);

    TransferSession session;
    session.id = makeId();
    session.fileName = fileName;
    session.fileSize = fileSize;
    session.cipher = cipher.empty() ? cfg.cipher : cipher;
    session.chunkSize = cfg.chunkSize;
    session.totalChunks = static_cast<int>((fileSize + cfg.chunkSize - 1) / cfg.chunkSize);
    if (session.totalChunks == 0) session.totalChunks = 1;
    session.sharedKey = std::to_string(shared);
    session.status = "transferring";
    session.path = std::string(STORE_DIR) + "/" + session.id + ".enc";

    std::ofstream out(session.path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("SecureFileTransfer: cannot create encrypted output file");

    std::string handshake = "DH|p=65537|g=3|client=" + std::to_string(clientPub) +
                            "|server=" + std::to_string(serverPub);
    std::string handshakeHash = Hash::compute(handshake, Hash::SHA1);
    std::string signature = signDigest(handshakeHash);
    std::string fingerprint = Hash::compute(session.sharedKey, Hash::SHA1).substr(0, 16);

    StartResult result;
    result.id = session.id;
    result.chunkSize = static_cast<int>(session.chunkSize);
    result.totalChunks = session.totalChunks;
    result.serverPubKey = std::to_string(serverPub);
    result.clientPubKey = std::to_string(clientPub);
    result.keyFingerprint = fingerprint;
    result.signature = signature;

    std::lock_guard<std::mutex> lock(g_mutex);
    g_sessions[session.id] = session;
    return result;
}

SecureFileTransfer::ChunkResult SecureFileTransfer::receiveChunk(
    const std::string& id,
    int chunkIndex,
    const std::string& chunkData)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    TransferSession& s = requireSession(id);
    if (s.status == "done") throw std::runtime_error("SecureFileTransfer: transfer already finished");
    if (chunkIndex != s.receivedChunks)
        throw std::runtime_error("SecureFileTransfer: chunks must be uploaded in order");

    std::string chunkHash = Hash::compute(chunkData, Hash::SHA1);
    std::string encrypted = xorCrypt(chunkData, s.sharedKey, s.id, chunkIndex);
    std::string encryptedHash = Hash::compute(encrypted, Hash::SHA1);

    std::ofstream out(s.path, std::ios::binary | std::ios::app);
    if (!out) {
        s.status = "error";
        throw std::runtime_error("SecureFileTransfer: cannot append encrypted chunk");
    }
    out.write(encrypted.data(), static_cast<std::streamsize>(encrypted.size()));
    if (!out) {
        s.status = "error";
        throw std::runtime_error("SecureFileTransfer: failed to write encrypted chunk");
    }

    s.manifest += std::to_string(chunkIndex) + ":" + chunkHash + ":" + encryptedHash + "\n";
    s.receivedChunks++;

    ChunkResult result;
    result.id = id;
    result.receivedChunks = s.receivedChunks;
    result.progress = std::min(99, static_cast<int>((100LL * s.receivedChunks) / s.totalChunks));
    result.chunkHash = chunkHash;
    result.encryptedHash = encryptedHash;
    return result;
}

SecureFileTransfer::UploadResult SecureFileTransfer::finishUpload(const std::string& id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    TransferSession& s = requireSession(id);
    if (s.receivedChunks != s.totalChunks)
        throw std::runtime_error("SecureFileTransfer: not all chunks have been received");

    s.finalHash = Hash::compute(s.manifest, Hash::SHA1);
    s.signature = signDigest(s.finalHash);
    s.status = "done";

    UploadResult result;
    result.id = id;
    result.chunks = s.totalChunks;
    result.hash = s.finalHash;
    result.signature = s.signature;
    result.path = s.path;
    return result;
}

SecureFileTransfer::Status SecureFileTransfer::getStatus(const std::string& id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    TransferSession& s = requireSession(id);
    Status status;
    status.id = id;
    status.status = s.status;
    status.progress = s.status == "done"
        ? 100
        : std::min(99, static_cast<int>((100LL * s.receivedChunks) / s.totalChunks));
    return status;
}

std::string SecureFileTransfer::getFile(const std::string& id) {
    std::string path = getFilePath(id);
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("SecureFileTransfer: cannot open encrypted file");
    std::ostringstream ss;
    ss << in.rdbuf();
    return ss.str();
}

std::string SecureFileTransfer::getFilePath(const std::string& id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    TransferSession& s = requireSession(id);
    if (s.status != "done") throw std::runtime_error("SecureFileTransfer: transfer is not complete");
    return s.path;
}
