#include "SecureFileTransfer.h"
#include "Hash.h"

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
    std::string path;
    std::string senderPubKey;
    std::string receiverPubKey;
    uint64_t fileSize = 0;
    size_t chunkSize = 4 * 1024 * 1024;
    int totalChunks = 0;
    int receivedChunks = 0;
    std::string status = "waiting_receiver";
    std::string cipher = "E2E-XOR";
};

static std::mutex g_mutex;
static std::unordered_map<std::string, TransferSession> g_sessions;
static const char* STORE_DIR = "transfers";

static std::string makeId() {
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch().count();
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;
    std::ostringstream oss;
    oss << "e2e_" << std::hex << now << "_" << dist(gen);
    return oss.str();
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

SecureFileTransfer::StartResult SecureFileTransfer::startUpload(
    const std::string& fileName,
    uint64_t fileSize,
    const std::string& senderPubKey,
    const Config& cfg)
{
    if (cfg.chunkSize == 0) throw std::runtime_error("SecureFileTransfer: chunk size must be positive");
    ensureStore();

    TransferSession session;
    session.id = makeId();
    session.fileName = fileName;
    session.fileSize = fileSize;
    session.senderPubKey = senderPubKey;
    session.chunkSize = cfg.chunkSize;
    session.totalChunks = static_cast<int>((fileSize + cfg.chunkSize - 1) / cfg.chunkSize);
    if (session.totalChunks == 0) session.totalChunks = 1;
    session.status = "waiting_receiver";
    session.path = std::string(STORE_DIR) + "/" + session.id + ".dat";

    // 创建（清空）密文存储文件
    std::ofstream out(session.path, std::ios::binary | std::ios::trunc);
    if (!out) throw std::runtime_error("SecureFileTransfer: cannot create storage file");

    StartResult result;
    result.id = session.id;
    result.chunkSize = static_cast<int>(session.chunkSize);
    result.totalChunks = session.totalChunks;
    result.status = session.status;

    std::lock_guard<std::mutex> lock(g_mutex);
    g_sessions[session.id] = session;
    return result;
}

SecureFileTransfer::JoinResult SecureFileTransfer::joinTransfer(const std::string& id, const std::string& receiverPubKey) {
    std::lock_guard<std::mutex> lock(g_mutex);
    TransferSession& s = requireSession(id);
    s.receiverPubKey = receiverPubKey;
    s.status = "paired";

    JoinResult result;
    result.senderPubKey = s.senderPubKey;
    result.fileName = s.fileName;
    result.fileSize = s.fileSize;
    result.totalChunks = s.totalChunks;
    result.chunkSize = static_cast<int>(s.chunkSize);
    return result;
}

SecureFileTransfer::ChunkResult SecureFileTransfer::receiveChunk(
    const std::string& id,
    int chunkIndex,
    const std::string& encryptedData)
{
    std::lock_guard<std::mutex> lock(g_mutex);
    TransferSession& s = requireSession(id);
    if (chunkIndex != s.receivedChunks)
        throw std::runtime_error("SecureFileTransfer: chunks must be uploaded in order");

    std::string encryptedHash = Hash::compute(encryptedData, Hash::SHA1);

    std::ofstream out(s.path, std::ios::binary | std::ios::app);
    if (!out) throw std::runtime_error("SecureFileTransfer: cannot append chunk");
    out.write(encryptedData.data(), static_cast<std::streamsize>(encryptedData.size()));
    
    s.receivedChunks++;
    if (s.receivedChunks == s.totalChunks) s.status = "done";
    else s.status = "transferring";

    ChunkResult result;
    result.id = id;
    result.receivedChunks = s.receivedChunks;
    result.progress = std::min(100, static_cast<int>((100LL * s.receivedChunks) / s.totalChunks));
    result.encryptedHash = encryptedHash;
    return result;
}

std::string SecureFileTransfer::getChunk(const std::string& id, int chunkIndex) {
    std::string path;
    size_t chunkSize;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        TransferSession& s = requireSession(id);
        path = s.path;
        chunkSize = s.chunkSize;
    }

    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("SecureFileTransfer: cannot open storage file");

    in.seekg(static_cast<std::streamoff>(chunkIndex * chunkSize));
    std::string buffer(chunkSize, '\0');
    in.read(&buffer[0], static_cast<std::streamsize>(chunkSize));
    buffer.resize(static_cast<size_t>(in.gcount()));
    return buffer;
}

void SecureFileTransfer::finishUpload(const std::string& id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    TransferSession& s = requireSession(id);
    s.status = "done";
}

SecureFileTransfer::TransferInfo SecureFileTransfer::getTransferInfo(const std::string& id) {
    std::lock_guard<std::mutex> lock(g_mutex);
    TransferSession& s = requireSession(id);
    TransferInfo info;
    info.id = s.id;
    info.fileName = s.fileName;
    info.fileSize = s.fileSize;
    info.totalChunks = s.totalChunks;
    info.receivedChunks = s.receivedChunks;
    info.status = s.status;
    info.senderPubKey = s.senderPubKey;
    info.receiverPubKey = s.receiverPubKey;
    info.cipher = s.cipher;
    return info;
}

std::vector<SecureFileTransfer::TransferInfo> SecureFileTransfer::listTransfers() {
    std::lock_guard<std::mutex> lock(g_mutex);
    std::vector<TransferInfo> list;
    for (const auto& pair : g_sessions) {
        const auto& s = pair.second;
        TransferInfo info;
        info.id = s.id;
        info.fileName = s.fileName;
        info.fileSize = s.fileSize;
        info.totalChunks = s.totalChunks;
        info.receivedChunks = s.receivedChunks;
        info.status = s.status;
        info.senderPubKey = s.senderPubKey;
        info.receiverPubKey = s.receiverPubKey;
        info.cipher = s.cipher;
        list.push_back(info);
    }
    return list;
}
