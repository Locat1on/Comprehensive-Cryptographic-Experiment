#pragma once
#include "BigInt128.h"
#include "RSA.h"
#include <vector>
#include <string>

class DHProtocol {
    BigInt128 p_, g_;
    BigInt128 privateKey_, publicKey_;
public:
    DHProtocol(BigInt128 p, BigInt128 g);

    BigInt128 generatePublicKey();
    BigInt128 computeSharedKey(const BigInt128& peerPublic);

    std::vector<uint8_t> signedExchange(const RSA& signingKey,
                                         const std::string& nonce);
    bool verifyPeer(const std::vector<uint8_t>& signedMsg,
                    const RSA::KeyPair& peerPubKey);
};
