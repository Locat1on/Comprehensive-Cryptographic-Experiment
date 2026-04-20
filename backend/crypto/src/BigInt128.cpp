#include "BigInt128.h"
#include <stdexcept>

BigInt128::BigInt128(uint64_t hi, uint64_t lo) : hi_(hi), lo_(lo) {}

BigInt128::BigInt128(const std::string& dec) {
    // TODO: 从十进制字符串构造
    throw std::runtime_error("BigInt128(dec) not implemented");
}

BigInt128 BigInt128::operator+(const BigInt128& rhs) const {
    // TODO: 低位相加，进位传高位
    throw std::runtime_error("BigInt128::operator+ not implemented");
}

BigInt128 BigInt128::operator-(const BigInt128& rhs) const {
    // TODO: 借位减法
    throw std::runtime_error("BigInt128::operator- not implemented");
}

BigInt256 BigInt128::operator*(const BigInt128& rhs) const {
    // TODO: Karatsuba 或长乘法，结果存入 BigInt256
    throw std::runtime_error("BigInt128::operator* not implemented");
}

bool BigInt128::operator==(const BigInt128& rhs) const {
    return hi_ == rhs.hi_ && lo_ == rhs.lo_;
}

bool BigInt128::operator<(const BigInt128& rhs) const {
    if (hi_ != rhs.hi_) return hi_ < rhs.hi_;
    return lo_ < rhs.lo_;
}

std::string BigInt128::toHex() const {
    // TODO: 返回 32 字符大写十六进制
    throw std::runtime_error("BigInt128::toHex not implemented");
}

std::string BigInt128::toDec() const {
    // TODO: 转十进制字符串
    throw std::runtime_error("BigInt128::toDec not implemented");
}

BigInt128 BigInt128::fromHex(const std::string& hex) {
    // TODO: 从十六进制字符串构造
    throw std::runtime_error("BigInt128::fromHex not implemented");
}

// ── BigInt256 ─────────────────────────────────────────────────

BigInt256::BigInt256() : w_{0, 0, 0, 0} {}

BigInt256::BigInt256(uint64_t w3, uint64_t w2, uint64_t w1, uint64_t w0)
    : w_{w0, w1, w2, w3} {}

std::string BigInt256::toHex() const {
    // TODO: 返回 64 字符大写十六进制
    throw std::runtime_error("BigInt256::toHex not implemented");
}

std::string BigInt256::toDec() const {
    // TODO: 转十进制字符串
    throw std::runtime_error("BigInt256::toDec not implemented");
}
