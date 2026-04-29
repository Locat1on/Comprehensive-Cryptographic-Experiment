#include "BigInt128.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <array>

// ── BigInt128 ─────────────────────────────────────────────────

BigInt128::BigInt128(uint64_t hi, uint64_t lo) : hi_(hi), lo_(lo) {}

BigInt128::BigInt128(const std::string& dec) : hi_(0), lo_(0) {
    if (dec.empty())
        throw std::runtime_error("BigInt128(dec): empty input");

    size_t idx = 0;
    bool negative = false;
    if (dec[0] == '+' || dec[0] == '-') {
        negative = (dec[0] == '-');
        idx = 1;
        if (idx == dec.size())
            throw std::runtime_error("BigInt128(dec): invalid decimal string");
    }

    const unsigned __int128 max128 = ~((unsigned __int128)0);
    unsigned __int128 value = 0;
    for (; idx < dec.size(); ++idx) {
        char c = dec[idx];
        if (c < '0' || c > '9')
            throw std::runtime_error("BigInt128(dec): invalid decimal string");
        unsigned digit = static_cast<unsigned>(c - '0');
        if (value > (max128 - digit) / 10)
            throw std::runtime_error("BigInt128(dec): overflow (>128 bits)");
        value = value * 10 + digit;
    }

    if (negative) {
        const unsigned __int128 maxAbs = (static_cast<unsigned __int128>(1) << 127);
        if (value > maxAbs)
            throw std::runtime_error("BigInt128(dec): overflow for negative value");
        value = (static_cast<unsigned __int128>(0) - value) & max128;
    }

    hi_ = static_cast<uint64_t>(value >> 64);
    lo_ = static_cast<uint64_t>(value);
}

BigInt128 BigInt128::operator+(const BigInt128& rhs) const {
    uint64_t lo = lo_ + rhs.lo_;
    uint64_t carry = (lo < lo_) ? 1ULL : 0ULL;
    uint64_t hi = hi_ + rhs.hi_ + carry;

    bool sign_a = (hi_ >> 63) != 0;
    bool sign_b = (rhs.hi_ >> 63) != 0;
    bool sign_r = (hi >> 63) != 0;
    if (sign_a == sign_b && sign_r != sign_a)
        throw std::runtime_error("BigInt128::operator+: overflow (>128 bits)");

    return BigInt128(hi, lo);
}

BigInt128 BigInt128::operator-(const BigInt128& rhs) const {
    uint64_t lo = lo_ - rhs.lo_;
    uint64_t borrow = (lo_ < rhs.lo_) ? 1ULL : 0ULL;
    uint64_t hi = hi_ - rhs.hi_ - borrow;
    return BigInt128(hi, lo);
}

BigInt256 BigInt128::operator*(const BigInt128& rhs) const {
    uint64_t a_lo = lo_, a_hi = hi_;
    uint64_t b_lo = rhs.lo_, b_hi = rhs.hi_;

    unsigned __int128 p00 = static_cast<unsigned __int128>(a_lo) * b_lo;
    unsigned __int128 p01 = static_cast<unsigned __int128>(a_lo) * b_hi;
    unsigned __int128 p10 = static_cast<unsigned __int128>(a_hi) * b_lo;
    unsigned __int128 p11 = static_cast<unsigned __int128>(a_hi) * b_hi;

    uint64_t w0 = static_cast<uint64_t>(p00);
    unsigned __int128 mid = (p00 >> 64) + p01 + p10;
    uint64_t w1 = static_cast<uint64_t>(mid);
    unsigned __int128 high = (mid >> 64) + p11;
    uint64_t w2 = static_cast<uint64_t>(high);
    uint64_t w3 = static_cast<uint64_t>(high >> 64);

    return BigInt256(w3, w2, w1, w0);
}

bool BigInt128::operator==(const BigInt128& rhs) const {
    return hi_ == rhs.hi_ && lo_ == rhs.lo_;
}

bool BigInt128::operator<(const BigInt128& rhs) const {
    if (hi_ != rhs.hi_) return hi_ < rhs.hi_;
    return lo_ < rhs.lo_;
}

std::string BigInt128::toHex() const {
    // 输出 32 字符大写十六进制（高 64 位 + 低 64 位）
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0')
        << std::setw(16) << hi_
        << std::setw(16) << lo_;
    return oss.str();
}

std::string BigInt128::toDec() const {
    unsigned __int128 val = (static_cast<unsigned __int128>(hi_) << 64) | lo_;
    if (val == 0)
        return "0";

    bool negative = (hi_ & (1ull << 63)) != 0;
    if (negative) {
        val = (~val) + 1;
    }

    std::string result;
    while (val > 0) {
        result += static_cast<char>('0' + static_cast<int>(val % 10));
        val /= 10;
    }
    std::reverse(result.begin(), result.end());
    if (negative)
        result.insert(result.begin(), '-');
    return result;
}

BigInt128 BigInt128::fromHex(const std::string& hex) {
    std::string h = hex;
    // 去除可选的 0x 前缀
    if (h.size() >= 2 && h[0] == '0' && (h[1] == 'x' || h[1] == 'X'))
        h = h.substr(2);
    if (h.size() > 32)
        throw std::runtime_error("BigInt128::fromHex: input exceeds 128 bits");
    // 补齐至 32 字符
    while (h.size() < 32) h = "0" + h;
    uint64_t hi = std::stoull(h.substr(0, 16), nullptr, 16);
    uint64_t lo = std::stoull(h.substr(16, 16), nullptr, 16);
    return BigInt128(hi, lo);
}

// ── BigInt256 ─────────────────────────────────────────────────

BigInt256::BigInt256() : w_{0, 0, 0, 0} {}

BigInt256::BigInt256(uint64_t w3, uint64_t w2, uint64_t w1, uint64_t w0)
    : w_{w0, w1, w2, w3} {}

std::string BigInt256::toHex() const {
    // 输出 64 字符大写十六进制（w_[3] 为最高位）
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0')
        << std::setw(16) << w_[3]
        << std::setw(16) << w_[2]
        << std::setw(16) << w_[1]
        << std::setw(16) << w_[0];
    return oss.str();
}

std::string BigInt256::toDec() const {
    if (!w_[0] && !w_[1] && !w_[2] && !w_[3]) return "0";
    // 256 位除 10 取余，逐位收集
    uint64_t arr[4] = {w_[0], w_[1], w_[2], w_[3]};
    std::string result;
    while (arr[0] || arr[1] || arr[2] || arr[3]) {
        uint64_t rem = 0;
        for (int i = 3; i >= 0; i--) {
            unsigned __int128 cur = ((unsigned __int128)rem << 64) | arr[i];
            arr[i] = (uint64_t)(cur / 10);
            rem    = (uint64_t)(cur % 10);
        }
        result += char('0' + rem);
    }
    std::reverse(result.begin(), result.end());
    return result;
}
