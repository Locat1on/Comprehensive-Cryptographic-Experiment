#include "DHProtocol.h"
#include "DigitalSignature.h"
#include "Hash.h"

#include <cstdint>
#include <random>
#include <sstream>
#include <stdexcept>

namespace {

static uint64_t toU64(const BigInt128& v) {
    return std::stoull(v.toDec());
}

static BigInt128 fromU64(uint64_t v) {
    return BigInt128(0ULL, v);
}

static uint64_t modPow(uint64_t base, uint64_t exp, uint64_t mod) {
    if (mod < 2) throw std::runtime_error("DHProtocol: modulus must be >= 2");
    uint64_t result = 1 % mod;
    base %= mod;
    while (exp > 0) {
        if (exp & 1ULL) result = static_cast<uint64_t>((unsigned __int128)result * base % mod);
        base = static_cast<uint64_t>((unsigned __int128)base * base % mod);
        exp >>= 1U;
    }
    return result;
}

static uint64_t randomPrivate(uint64_t p) {
    uint64_t upper = p > 4 ? p - 2 : 2;
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<uint64_t> dist(2, upper);
    return dist(gen);
}

static std::string makePayload(const BigInt128& p, const BigInt128& g,
                               const BigInt128& pub, const std::string& nonce) {
    return "p=" + p.toDec() + ";g=" + g.toDec() + ";pub=" + pub.toDec() + ";nonce=" + nonce;
}

} // namespace

DHProtocol::DHProtocol(BigInt128 p, BigInt128 g) : p_(p), g_(g) {
    uint64_t p64 = toU64(p_);
    if (p64 < 5) throw std::runtime_error("DHProtocol: p is too small");
    uint64_t g64 = toU64(g_);
    if (g64 < 2 || g64 >= p64) throw std::runtime_error("DHProtocol: g must be in [2, p)");
    privateKey_ = fromU64(randomPrivate(p64));
    publicKey_ = fromU64(0);
}

BigInt128 DHProtocol::generatePublicKey() {
    uint64_t p = toU64(p_);
    uint64_t g = toU64(g_);
    uint64_t a = toU64(privateKey_);
    publicKey_ = fromU64(modPow(g, a, p));
    return publicKey_;
}

BigInt128 DHProtocol::computeSharedKey(const BigInt128& peerPublic) {
    uint64_t p = toU64(p_);
    uint64_t peer = toU64(peerPublic);
    if (peer <= 1 || peer >= p) throw std::runtime_error("DHProtocol: invalid peer public key");
    return fromU64(modPow(peer, toU64(privateKey_), p));
}

std::vector<uint8_t> DHProtocol::signedExchange(const RSA& signingKey,
                                                const std::string& nonce) {
    if (publicKey_ == BigInt128(0ULL, 0ULL))
        generatePublicKey();

    std::string payload = makePayload(p_, g_, publicKey_, nonce);
    RSA signer = signingKey;
    const std::string digest = Hash::compute(payload, Hash::SHA1);
    BigInt128 digestInt = BigInt128::fromHex(digest.substr(digest.size() - 32));
    const std::string sig = signer.sign(digestInt).toHex();
    std::string packet = payload + "\n" + sig;
    return std::vector<uint8_t>(packet.begin(), packet.end());
}

bool DHProtocol::verifyPeer(const std::vector<uint8_t>& signedMsg,
                            const RSA::KeyPair& peerPubKey) {
    std::string packet(signedMsg.begin(), signedMsg.end());
    size_t split = packet.rfind('\n');
    if (split == std::string::npos) return false;
    std::string payload = packet.substr(0, split);
    std::string sigHex = packet.substr(split + 1);

    const std::string digest = Hash::compute(payload, Hash::SHA1);
    BigInt128 digestInt = BigInt128::fromHex(digest.substr(digest.size() - 32));
    RSA verifier(peerPubKey);
    return verifier.verify(digestInt, BigInt128::fromHex(sigHex));
}
