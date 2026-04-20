#pragma once
#include <string>
#include <cstdint>

class BigInt256;

class BigInt128 {
    uint64_t hi_, lo_;
public:
    BigInt128(uint64_t hi = 0, uint64_t lo = 0);
    explicit BigInt128(const std::string& dec);

    BigInt128 operator+(const BigInt128& rhs) const;
    BigInt128 operator-(const BigInt128& rhs) const;
    BigInt256 operator*(const BigInt128& rhs) const;

    bool operator==(const BigInt128& rhs) const;
    bool operator< (const BigInt128& rhs) const;

    std::string toHex() const;
    std::string toDec() const;
    static BigInt128 fromHex(const std::string& hex);
};

class BigInt256 {
    uint64_t w_[4];  // w_[0] = lowest 64 bits
public:
    BigInt256();
    BigInt256(uint64_t w3, uint64_t w2, uint64_t w1, uint64_t w0);

    std::string toHex() const;
    std::string toDec() const;
};
