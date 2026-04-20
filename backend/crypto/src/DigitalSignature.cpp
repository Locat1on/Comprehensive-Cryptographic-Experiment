#include "DigitalSignature.h"
#include <stdexcept>

DigitalSignature::DigitalSignature(const RSA::KeyPair& kp) : rsa_(kp) {}

std::string DigitalSignature::sign(const std::string& msg, Hash::Algo algo) {
    // TODO: H = Hash::compute(msg, algo); S = rsa_.sign(BigInt128(H)); return S.toHex()
    throw std::runtime_error("DigitalSignature::sign not implemented");
}

bool DigitalSignature::verify(const std::string& msg,
                               const std::string& signature,
                               Hash::Algo algo) {
    // TODO: H = Hash::compute(msg, algo); return rsa_.verify(BigInt128(H), BigInt128::fromHex(signature))
    throw std::runtime_error("DigitalSignature::verify not implemented");
}
