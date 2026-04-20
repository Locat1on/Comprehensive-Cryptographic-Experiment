#include "RSA.h"
#include <stdexcept>

RSA::KeyPair RSA::generate(uint16_t p, uint16_t q, uint16_t e) {
    // TODO: n=p*q, phi=(p-1)*(q-1), 验证 gcd(e,phi)=1, 求 d=e⁻¹ mod phi
    throw std::runtime_error("RSA::generate not implemented");
}

RSA::RSA(const KeyPair& kp) : kp_(kp) {}

std::vector<BigInt128> RSA::encrypt(const std::string& msg) {
    // TODO: 每字节 m 计算 c = m^e mod n
    throw std::runtime_error("RSA::encrypt not implemented");
}

std::string RSA::decrypt(const std::vector<BigInt128>& blocks) {
    // TODO: 每块 c 计算 m = c^d mod n，拼接为字符串
    throw std::runtime_error("RSA::decrypt not implemented");
}

BigInt128 RSA::sign(const BigInt128& hash) {
    // TODO: S = hash^d mod n
    throw std::runtime_error("RSA::sign not implemented");
}

bool RSA::verify(const BigInt128& hash, const BigInt128& sig) {
    // TODO: hash == sig^e mod n
    throw std::runtime_error("RSA::verify not implemented");
}
