#include "RSA.h"
#include <stdexcept>
#include <string>

// ── 内部辅助函数 ──────────────────────────────────────────────

// 快速模幂：base^exp mod mod（所有值均 < 2^32，中间乘积 < 2^64）
static uint64_t modpow(uint64_t base, uint64_t exp, uint64_t mod) {
    if (mod == 1) return 0;
    uint64_t result = 1;
    base %= mod;
    while (exp > 0) {
        if (exp & 1) result = result * base % mod;
        base = base * base % mod;
        exp >>= 1;
    }
    return result;
}

// 扩展欧几里得：返回 gcd(a,b)，同时令 a*x ≡ gcd (mod b)
static int64_t extgcd(int64_t a, int64_t b, int64_t& x, int64_t& y) {
    if (b == 0) { x = 1; y = 0; return a; }
    int64_t x1, y1;
    int64_t g = extgcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - (a / b) * y1;
    return g;
}

// 最大公因数（uint64_t）
static uint64_t gcd64(uint64_t a, uint64_t b) {
    while (b) { uint64_t t = b; b = a % b; a = t; }
    return a;
}

// Miller-Rabin 单轮素性检验（见证数 a）
static bool millerRabin(uint64_t n, uint64_t a) {
    if (n % a == 0) return n == a;
    uint64_t d = n - 1;
    int r = 0;
    while (d % 2 == 0) { d /= 2; ++r; }
    uint64_t x = modpow(a, d, n);
    if (x == 1 || x == n - 1) return true;
    for (int j = 0; j < r - 1; ++j) {
        x = modpow(x, 2, n);
        if (x == n - 1) return true;
    }
    return false;
}

// 对 uint16_t 范围（n < 65536），见证 {2, 3} 给出确定性结果（覆盖 n < 1,373,653）
static bool isPrime(uint16_t n) {
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0 || n % 3 == 0) return false;
    return millerRabin((uint64_t)n, 2) && millerRabin((uint64_t)n, 3);
}

// 将 BigInt128 转为 uint64_t（RSA 中 n < 2^16，值很小）
static uint64_t toLong(const BigInt128& v) {
    std::string dec = v.toDec();
    if (dec == "0") return 0ULL;
    return std::stoull(dec);
}

// 将 uint64_t 包装为 BigInt128
static BigInt128 fromLong(uint64_t v) {
    return BigInt128(0ULL, v);
}

// 计算 BigInt128（最大 128 位）对小模数 n 的余数
static uint64_t bigmod(const BigInt128& v, uint64_t n) {
    if (n == 0) throw std::runtime_error("RSA: modulus is zero");
    // v = hi * 2^64 + lo  =>  v mod n = ((hi mod n)*(2^64 mod n) + lo mod n) mod n
    std::string hex = v.toHex();                                // 32 位大写十六进制
    uint64_t hi = std::stoull(hex.substr(0, 16), nullptr, 16);
    uint64_t lo = std::stoull(hex.substr(16, 16), nullptr, 16);
    uint64_t pow64 = modpow(2ULL, 64ULL, n);                   // 2^64 mod n
    return ((hi % n) * pow64 % n + lo % n) % n;
}

// ── RSA 实现 ──────────────────────────────────────────────────

RSA::KeyPair RSA::generate(uint16_t p, uint16_t q, uint16_t e) {
    if (p < 2 || q < 2)
        throw std::runtime_error("RSA::generate: p and q must be >= 2");
    if (!isPrime(p) || !isPrime(q))
        throw std::runtime_error("RSA::generate: p and q must be prime numbers");

    uint64_t n   = (uint64_t)p * q;
    uint64_t phi = (uint64_t)(p - 1) * (q - 1);

    if (gcd64((uint64_t)e, phi) != 1)
        throw std::runtime_error("RSA::generate: gcd(e, phi(n)) != 1, choose a different e");

    // 用扩展欧几里得求 d ≡ e⁻¹ (mod phi)
    int64_t x, y;
    extgcd((int64_t)e, (int64_t)phi, x, y);
    int64_t d = ((x % (int64_t)phi) + (int64_t)phi) % (int64_t)phi;

    KeyPair kp;
    kp.n = fromLong(n);
    kp.e = fromLong((uint64_t)e);
    kp.d = fromLong((uint64_t)d);
    return kp;
}

RSA::RSA(const KeyPair& kp) : kp_(kp) {}

std::vector<BigInt128> RSA::encrypt(const std::string& msg) {
    uint64_t n = toLong(kp_.n);
    uint64_t e = toLong(kp_.e);

    std::vector<BigInt128> result;
    result.reserve(msg.size());
    for (unsigned char c : msg) {
        uint64_t m = c;
        if (m >= n)
            throw std::runtime_error("RSA::encrypt: byte value >= n, use larger primes");
        // c = m^e mod n
        result.push_back(fromLong(modpow(m, e, n)));
    }
    return result;
}

std::string RSA::decrypt(const std::vector<BigInt128>& blocks) {
    uint64_t n = toLong(kp_.n);
    uint64_t d = toLong(kp_.d);

    std::string result;
    result.reserve(blocks.size());
    for (const auto& block : blocks) {
        uint64_t c = toLong(block);
        // m = c^d mod n
        result += (char)(uint8_t)modpow(c, d, n);
    }
    return result;
}

BigInt128 RSA::sign(const BigInt128& hash) {
    uint64_t n = toLong(kp_.n);
    uint64_t d = toLong(kp_.d);
    // S = (hash mod n)^d mod n
    uint64_t h = bigmod(hash, n);
    return fromLong(modpow(h, d, n));
}

bool RSA::verify(const BigInt128& hash, const BigInt128& sig) {
    uint64_t n = toLong(kp_.n);
    uint64_t e = toLong(kp_.e);
    uint64_t h = bigmod(hash, n);
    uint64_t s = toLong(sig);
    // 验证 sig^e mod n == hash mod n
    return h == modpow(s, e, n);
}
