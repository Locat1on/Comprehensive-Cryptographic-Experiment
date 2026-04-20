#pragma once
#include "BigInt128.h"
#include <vector>
#include <string>
#include <cstdint>

class RSA {
public:
    struct KeyPair {
        BigInt128 n, e, d;
    };

    static KeyPair generate(uint16_t p, uint16_t q, uint16_t e);
    explicit RSA(const KeyPair& kp);

    std::vector<BigInt128> encrypt(const std::string& msg);
    std::string            decrypt(const std::vector<BigInt128>& blocks);

    BigInt128 sign  (const BigInt128& hash);
    bool      verify(const BigInt128& hash, const BigInt128& sig);

private:
    KeyPair kp_;
};
