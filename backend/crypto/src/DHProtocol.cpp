#include "DHProtocol.h"
#include <stdexcept>

DHProtocol::DHProtocol(BigInt128 p, BigInt128 g) : p_(p), g_(g) {}

BigInt128 DHProtocol::generatePublicKey() {
    // TODO: 随机生成私钥 a，计算 A = g^a mod p
    throw std::runtime_error("DHProtocol::generatePublicKey not implemented");
}

BigInt128 DHProtocol::computeSharedKey(const BigInt128& peerPublic) {
    // TODO: K = peerPublic^privateKey mod p
    throw std::runtime_error("DHProtocol::computeSharedKey not implemented");
}

std::vector<uint8_t> DHProtocol::signedExchange(const RSA& signingKey,
                                                  const std::string& nonce) {
    // TODO: 序列化 (p, g, publicKey, nonce)，RSA 签名后返回
    throw std::runtime_error("DHProtocol::signedExchange not implemented");
}

bool DHProtocol::verifyPeer(const std::vector<uint8_t>& signedMsg,
                             const RSA::KeyPair& peerPubKey) {
    // TODO: 反序列化，RSA 验证签名，校验 nonce 防重放
    throw std::runtime_error("DHProtocol::verifyPeer not implemented");
}
