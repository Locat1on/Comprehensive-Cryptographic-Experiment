#include "DigitalSignature.h"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace {

static std::string normalizeHex(std::string hex) {
    hex.erase(std::remove_if(hex.begin(), hex.end(), [](unsigned char c) {
        return std::isspace(c) != 0;
    }), hex.end());
    if (hex.size() >= 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X'))
        hex = hex.substr(2);
    std::transform(hex.begin(), hex.end(), hex.begin(), [](unsigned char c) {
        return static_cast<char>(std::toupper(c));
    });
    return hex;
}

static BigInt128 digestToBigInt(const std::string& digestHex) {
    std::string h = normalizeHex(digestHex);
    if (h.empty()) throw std::runtime_error("DigitalSignature: empty digest");
    if (h.size() > 32) h = h.substr(h.size() - 32);
    return BigInt128::fromHex(h);
}

} // namespace

DigitalSignature::DigitalSignature(const RSA::KeyPair& kp) : rsa_(kp) {}

std::string DigitalSignature::sign(const std::string& msg, Hash::Algo algo) {
    const std::string h = Hash::compute(msg, algo);
    return rsa_.sign(digestToBigInt(h)).toHex();
}

bool DigitalSignature::verify(const std::string& msg,
                              const std::string& signature,
                              Hash::Algo algo) {
    const std::string h = Hash::compute(msg, algo);
    const std::string sigHex = normalizeHex(signature);
    return rsa_.verify(digestToBigInt(h), BigInt128::fromHex(sigHex));
}
