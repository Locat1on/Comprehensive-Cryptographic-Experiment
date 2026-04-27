#include "Hash.h"

#include <array>
#include <algorithm>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace {

static uint32_t rol32(uint32_t v, uint32_t n) {
    return (v << n) | (v >> (32 - n));
}

static std::string hexBytes(const uint8_t* data, size_t len) {
    std::ostringstream oss;
    oss << std::uppercase << std::hex << std::setfill('0');
    for (size_t i = 0; i < len; ++i)
        oss << std::setw(2) << static_cast<int>(data[i]);
    return oss.str();
}

class SHA1Context {
public:
    SHA1Context() {
        h_[0] = 0x67452301U;
        h_[1] = 0xEFCDAB89U;
        h_[2] = 0x98BADCFEU;
        h_[3] = 0x10325476U;
        h_[4] = 0xC3D2E1F0U;
    }

    void update(const uint8_t* data, size_t len) {
        bitLen_ += static_cast<uint64_t>(len) * 8ULL;
        while (len > 0) {
            size_t take = std::min(len, block_.size() - used_);
            std::copy(data, data + take, block_.begin() + used_);
            used_ += take;
            data += take;
            len -= take;
            if (used_ == block_.size()) {
                transform(block_.data());
                used_ = 0;
            }
        }
    }

    std::string final() {
        block_[used_++] = 0x80;
        if (used_ > 56) {
            while (used_ < 64) block_[used_++] = 0;
            transform(block_.data());
            used_ = 0;
        }
        while (used_ < 56) block_[used_++] = 0;
        for (int i = 7; i >= 0; --i)
            block_[used_++] = static_cast<uint8_t>((bitLen_ >> (i * 8)) & 0xFF);
        transform(block_.data());

        uint8_t out[20];
        for (int i = 0; i < 5; ++i) {
            out[i * 4 + 0] = static_cast<uint8_t>((h_[i] >> 24) & 0xFF);
            out[i * 4 + 1] = static_cast<uint8_t>((h_[i] >> 16) & 0xFF);
            out[i * 4 + 2] = static_cast<uint8_t>((h_[i] >> 8) & 0xFF);
            out[i * 4 + 3] = static_cast<uint8_t>(h_[i] & 0xFF);
        }
        return hexBytes(out, sizeof(out));
    }

private:
    void transform(const uint8_t* b) {
        uint32_t w[80];
        for (int i = 0; i < 16; ++i) {
            w[i] = (static_cast<uint32_t>(b[i * 4]) << 24) |
                   (static_cast<uint32_t>(b[i * 4 + 1]) << 16) |
                   (static_cast<uint32_t>(b[i * 4 + 2]) << 8) |
                   static_cast<uint32_t>(b[i * 4 + 3]);
        }
        for (int i = 16; i < 80; ++i)
            w[i] = rol32(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);

        uint32_t a = h_[0], c = h_[2], d = h_[3], e = h_[4];
        uint32_t bb = h_[1];
        for (int i = 0; i < 80; ++i) {
            uint32_t f, k;
            if (i < 20) {
                f = (bb & c) | ((~bb) & d);
                k = 0x5A827999U;
            } else if (i < 40) {
                f = bb ^ c ^ d;
                k = 0x6ED9EBA1U;
            } else if (i < 60) {
                f = (bb & c) | (bb & d) | (c & d);
                k = 0x8F1BBCDCU;
            } else {
                f = bb ^ c ^ d;
                k = 0xCA62C1D6U;
            }
            uint32_t temp = rol32(a, 5) + f + e + k + w[i];
            e = d;
            d = c;
            c = rol32(bb, 30);
            bb = a;
            a = temp;
        }
        h_[0] += a;
        h_[1] += bb;
        h_[2] += c;
        h_[3] += d;
        h_[4] += e;
    }

    uint32_t h_[5]{};
    uint64_t bitLen_ = 0;
    std::array<uint8_t, 64> block_{};
    size_t used_ = 0;
};

class MD5Context {
public:
    MD5Context() {
        h_[0] = 0x67452301U;
        h_[1] = 0xEFCDAB89U;
        h_[2] = 0x98BADCFEU;
        h_[3] = 0x10325476U;
    }

    void update(const uint8_t* data, size_t len) {
        bitLen_ += static_cast<uint64_t>(len) * 8ULL;
        while (len > 0) {
            size_t take = std::min(len, block_.size() - used_);
            std::copy(data, data + take, block_.begin() + used_);
            used_ += take;
            data += take;
            len -= take;
            if (used_ == block_.size()) {
                transform(block_.data());
                used_ = 0;
            }
        }
    }

    std::string final() {
        block_[used_++] = 0x80;
        if (used_ > 56) {
            while (used_ < 64) block_[used_++] = 0;
            transform(block_.data());
            used_ = 0;
        }
        while (used_ < 56) block_[used_++] = 0;
        for (int i = 0; i < 8; ++i)
            block_[used_++] = static_cast<uint8_t>((bitLen_ >> (i * 8)) & 0xFF);
        transform(block_.data());

        uint8_t out[16];
        for (int i = 0; i < 4; ++i) {
            out[i * 4 + 0] = static_cast<uint8_t>(h_[i] & 0xFF);
            out[i * 4 + 1] = static_cast<uint8_t>((h_[i] >> 8) & 0xFF);
            out[i * 4 + 2] = static_cast<uint8_t>((h_[i] >> 16) & 0xFF);
            out[i * 4 + 3] = static_cast<uint8_t>((h_[i] >> 24) & 0xFF);
        }
        return hexBytes(out, sizeof(out));
    }

private:
    static uint32_t f(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (~x & z); }
    static uint32_t g(uint32_t x, uint32_t y, uint32_t z) { return (x & z) | (y & ~z); }
    static uint32_t h(uint32_t x, uint32_t y, uint32_t z) { return x ^ y ^ z; }
    static uint32_t i(uint32_t x, uint32_t y, uint32_t z) { return y ^ (x | ~z); }

    static void step(uint32_t& a, uint32_t b, uint32_t c, uint32_t d,
                     uint32_t x, uint32_t s, uint32_t ac,
                     uint32_t (*fn)(uint32_t, uint32_t, uint32_t)) {
        a = b + rol32(a + fn(b, c, d) + x + ac, s);
    }

    void transform(const uint8_t* b) {
        uint32_t x[16];
        for (int j = 0; j < 16; ++j) {
            x[j] = static_cast<uint32_t>(b[j * 4]) |
                   (static_cast<uint32_t>(b[j * 4 + 1]) << 8) |
                   (static_cast<uint32_t>(b[j * 4 + 2]) << 16) |
                   (static_cast<uint32_t>(b[j * 4 + 3]) << 24);
        }

        uint32_t a = h_[0], b0 = h_[1], c = h_[2], d = h_[3];

        step(a, b0, c, d, x[0], 7, 0xd76aa478U, f); step(d, a, b0, c, x[1], 12, 0xe8c7b756U, f);
        step(c, d, a, b0, x[2], 17, 0x242070dbU, f); step(b0, c, d, a, x[3], 22, 0xc1bdceeeU, f);
        step(a, b0, c, d, x[4], 7, 0xf57c0fafU, f); step(d, a, b0, c, x[5], 12, 0x4787c62aU, f);
        step(c, d, a, b0, x[6], 17, 0xa8304613U, f); step(b0, c, d, a, x[7], 22, 0xfd469501U, f);
        step(a, b0, c, d, x[8], 7, 0x698098d8U, f); step(d, a, b0, c, x[9], 12, 0x8b44f7afU, f);
        step(c, d, a, b0, x[10], 17, 0xffff5bb1U, f); step(b0, c, d, a, x[11], 22, 0x895cd7beU, f);
        step(a, b0, c, d, x[12], 7, 0x6b901122U, f); step(d, a, b0, c, x[13], 12, 0xfd987193U, f);
        step(c, d, a, b0, x[14], 17, 0xa679438eU, f); step(b0, c, d, a, x[15], 22, 0x49b40821U, f);

        step(a, b0, c, d, x[1], 5, 0xf61e2562U, g); step(d, a, b0, c, x[6], 9, 0xc040b340U, g);
        step(c, d, a, b0, x[11], 14, 0x265e5a51U, g); step(b0, c, d, a, x[0], 20, 0xe9b6c7aaU, g);
        step(a, b0, c, d, x[5], 5, 0xd62f105dU, g); step(d, a, b0, c, x[10], 9, 0x02441453U, g);
        step(c, d, a, b0, x[15], 14, 0xd8a1e681U, g); step(b0, c, d, a, x[4], 20, 0xe7d3fbc8U, g);
        step(a, b0, c, d, x[9], 5, 0x21e1cde6U, g); step(d, a, b0, c, x[14], 9, 0xc33707d6U, g);
        step(c, d, a, b0, x[3], 14, 0xf4d50d87U, g); step(b0, c, d, a, x[8], 20, 0x455a14edU, g);
        step(a, b0, c, d, x[13], 5, 0xa9e3e905U, g); step(d, a, b0, c, x[2], 9, 0xfcefa3f8U, g);
        step(c, d, a, b0, x[7], 14, 0x676f02d9U, g); step(b0, c, d, a, x[12], 20, 0x8d2a4c8aU, g);

        step(a, b0, c, d, x[5], 4, 0xfffa3942U, h); step(d, a, b0, c, x[8], 11, 0x8771f681U, h);
        step(c, d, a, b0, x[11], 16, 0x6d9d6122U, h); step(b0, c, d, a, x[14], 23, 0xfde5380cU, h);
        step(a, b0, c, d, x[1], 4, 0xa4beea44U, h); step(d, a, b0, c, x[4], 11, 0x4bdecfa9U, h);
        step(c, d, a, b0, x[7], 16, 0xf6bb4b60U, h); step(b0, c, d, a, x[10], 23, 0xbebfbc70U, h);
        step(a, b0, c, d, x[13], 4, 0x289b7ec6U, h); step(d, a, b0, c, x[0], 11, 0xeaa127faU, h);
        step(c, d, a, b0, x[3], 16, 0xd4ef3085U, h); step(b0, c, d, a, x[6], 23, 0x04881d05U, h);
        step(a, b0, c, d, x[9], 4, 0xd9d4d039U, h); step(d, a, b0, c, x[12], 11, 0xe6db99e5U, h);
        step(c, d, a, b0, x[15], 16, 0x1fa27cf8U, h); step(b0, c, d, a, x[2], 23, 0xc4ac5665U, h);

        step(a, b0, c, d, x[0], 6, 0xf4292244U, i); step(d, a, b0, c, x[7], 10, 0x432aff97U, i);
        step(c, d, a, b0, x[14], 15, 0xab9423a7U, i); step(b0, c, d, a, x[5], 21, 0xfc93a039U, i);
        step(a, b0, c, d, x[12], 6, 0x655b59c3U, i); step(d, a, b0, c, x[3], 10, 0x8f0ccc92U, i);
        step(c, d, a, b0, x[10], 15, 0xffeff47dU, i); step(b0, c, d, a, x[1], 21, 0x85845dd1U, i);
        step(a, b0, c, d, x[8], 6, 0x6fa87e4fU, i); step(d, a, b0, c, x[15], 10, 0xfe2ce6e0U, i);
        step(c, d, a, b0, x[6], 15, 0xa3014314U, i); step(b0, c, d, a, x[13], 21, 0x4e0811a1U, i);
        step(a, b0, c, d, x[4], 6, 0xf7537e82U, i); step(d, a, b0, c, x[11], 10, 0xbd3af235U, i);
        step(c, d, a, b0, x[2], 15, 0x2ad7d2bbU, i); step(b0, c, d, a, x[9], 21, 0xeb86d391U, i);

        h_[0] += a;
        h_[1] += b0;
        h_[2] += c;
        h_[3] += d;
    }

    uint32_t h_[4]{};
    uint64_t bitLen_ = 0;
    std::array<uint8_t, 64> block_{};
    size_t used_ = 0;
};

template <typename Context>
static std::string computeWithContext(const std::string& msg) {
    Context ctx;
    ctx.update(reinterpret_cast<const uint8_t*>(msg.data()), msg.size());
    return ctx.final();
}

template <typename Context>
static std::string computeFileWithContext(const std::string& path) {
    std::ifstream in(path, std::ios::binary);
    if (!in) throw std::runtime_error("Hash::computeFile: cannot open file");

    Context ctx;
    std::vector<char> buf(1024 * 1024);
    while (in) {
        in.read(buf.data(), static_cast<std::streamsize>(buf.size()));
        std::streamsize got = in.gcount();
        if (got > 0)
            ctx.update(reinterpret_cast<const uint8_t*>(buf.data()), static_cast<size_t>(got));
    }
    return ctx.final();
}

} // namespace

std::string Hash::compute(const std::string& msg, Algo algo) {
    return algo == MD5 ? computeWithContext<MD5Context>(msg)
                       : computeWithContext<SHA1Context>(msg);
}

std::string Hash::computeFile(const std::string& path, Algo algo) {
    return algo == MD5 ? computeFileWithContext<MD5Context>(path)
                       : computeFileWithContext<SHA1Context>(path);
}
