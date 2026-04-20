#include "AffineCipher.h"
#include <stdexcept>

AffineCipher::AffineCipher(int a, int b) : a_(a), b_(b) {}

AffineCipher AffineCipher::fromXML(const std::string& xmlPath) {
    // TODO: 解析 <key><a>...</a><b>...</b></key>
    throw std::runtime_error("AffineCipher::fromXML not implemented");
}

std::string AffineCipher::encrypt(const std::string& plaintext) {
    // TODO: E(x) = (a*x + b) mod 26
    throw std::runtime_error("AffineCipher::encrypt not implemented");
}

std::string AffineCipher::decrypt(const std::string& ciphertext) {
    // TODO: D(x) = a_inv * (x - b) mod 26
    throw std::runtime_error("AffineCipher::decrypt not implemented");
}

int AffineCipher::modInverse(int a, int m) {
    // TODO: 扩展欧几里得算法
    throw std::runtime_error("AffineCipher::modInverse not implemented");
}
