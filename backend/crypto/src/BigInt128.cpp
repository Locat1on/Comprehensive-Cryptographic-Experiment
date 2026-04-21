#include "BigInt128.h"
#include <stdexcept>
#include <algorithm>
#include <sstream>
#include <iomanip>

// ── BigInt128 ─────────────────────────────────────────────────

BigInt128::BigInt128(uint64_t hi, uint64_t lo) : hi_(hi), lo_(lo) {}

BigInt128::BigInt128(const std::string& dec) : hi_(0), lo_(0) {
    // 逐位解析十进制字符串，跳过非数字字符
    for (char c : dec) {
        if (c < '0' || c > '9') continue;
        // this = this * 10 + digit，使用 unsigned __int128 处理进位
        unsigned __int128 val = ((unsigned __int128)hi_ << 64) | lo_;
        val = val * 10 + (unsigned)(c - '0');
        hi_ = (uint64_t)(val >> 64);
        lo_ = (uint64_t)val;
    }
}

BigInt128 BigInt128::operator+(const BigInt128& rhs) const {
    uint64_t lo = lo_ + rhs.lo_;
    uint64_t carry = (lo < lo_) ? 1ULL : 0ULL;
    uint64_t hi = hi_ + rhs.hi_ + carry;
    return BigInt128(hi, lo);
}

BigInt128 BigInt128::operator-(const BigInt128& rhs) const {
    uint64_t lo = lo_ - rhs.lo_;
    uint64_t borrow = (lo_ < rhs.lo_) ? 1ULL : 0ULL;
    uint64_t hi = hi_ - rhs.hi_ - borrow;
    return BigInt128(hi, lo);
}

BigInt256 BigInt128::operator*(const BigInt128& rhs) const {
    // 将两个 128 位数各拆为两个 64 位半段，做四次 64x64->128 乘法后合并
    uint64_t a_lo = lo_, a_hi = hi_;
    uint64_t b_lo = rhs.lo_, b_hi = rhs.hi_;

    unsigned __int128 p00 = (unsigned __int128)a_lo * b_lo;   // 贡献 bits 0..127
    unsigned __int128 p01 = (unsigned __int128)a_lo * b_hi;   // 贡献 bits 64..191
    unsigned __int128 p10 = (unsigned __int128)a_hi * b_lo;   // 贡献 bits 64..191
    unsigned __int128 p11 = (unsigned __int128)a_hi * b_hi;   // 贡献 bits 128..255

    uint64_t w0 = (uint64_t)p00;
    unsigned __int128 mid = (p00 >> 64) + p01 + p10;
    uint64_t w1 = (uint64_t)mid;
    unsigned __int128 hi2 = (mid >> 64) + p11;
    uint64_t w2 = (uint64_t)hi2;
    uint64_t w3 = (uint64_t)(hi2 >> 64);

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
    if (hi_ == 0 && lo_ == 0) return "0";
    // 通过反复除 10 收集余数
    unsigned __int128 val = ((unsigned __int128)hi_ << 64) | lo_;
    std::string result;
    while (val > 0) {
        result += char('0' + (int)(val % 10));
        val /= 10;
    }
    std::reverse(result.begin(), result.end());
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
