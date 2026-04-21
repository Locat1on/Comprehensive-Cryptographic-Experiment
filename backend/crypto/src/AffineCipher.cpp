#include "AffineCipher.h"
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <regex>
#include <cctype>

AffineCipher::AffineCipher(int a, int b) : a_(a), b_(b) {
    if (a <= 0)
        throw std::runtime_error("AffineCipher: a must be positive");
    // 验证 gcd(a, 26) == 1
    int g = a % 26, tmp = 26;
    while (g) { int t = g; g = tmp % g; tmp = t; }
    if (tmp != 1)
        throw std::runtime_error("AffineCipher: a must be coprime with 26");
}

AffineCipher AffineCipher::fromXML(const std::string& xmlPath) {
    std::ifstream f(xmlPath);
    if (!f) throw std::runtime_error("AffineCipher::fromXML: cannot open " + xmlPath);
    std::ostringstream ss;
    ss << f.rdbuf();
    std::string content = ss.str();

    std::smatch ma, mb;
    std::regex ra("<a>(\\d+)</a>"), rb("<b>(\\d+)</b>");
    if (!std::regex_search(content, ma, ra) || !std::regex_search(content, mb, rb))
        throw std::runtime_error("AffineCipher::fromXML: missing <a> or <b> tags");

    return AffineCipher(std::stoi(ma[1]), std::stoi(mb[1]));
}

std::string AffineCipher::encrypt(const std::string& plaintext) {
    std::string result;
    result.reserve(plaintext.size());
    for (unsigned char c : plaintext) {
        if (std::isalpha(c)) {
            // 统一转为大写后加密，保持大小写
            bool lower = std::islower(c);
            int x = std::toupper(c) - 'A';
            int enc = (a_ * x + b_) % 26;
            result += lower ? (char)('a' + enc) : (char)('A' + enc);
        } else {
            result += (char)c;
        }
    }
    return result;
}

std::string AffineCipher::decrypt(const std::string& ciphertext) {
    int a_inv = modInverse(a_, 26);
    std::string result;
    result.reserve(ciphertext.size());
    for (unsigned char c : ciphertext) {
        if (std::isalpha(c)) {
            bool lower = std::islower(c);
            int y = std::toupper(c) - 'A';
            int dec = (a_inv * ((y - b_) % 26 + 26)) % 26;
            result += lower ? (char)('a' + dec) : (char)('A' + dec);
        } else {
            result += (char)c;
        }
    }
    return result;
}

int AffineCipher::modInverse(int a, int m) {
    // 扩展欧几里得：找 x 使得 a*x ≡ 1 (mod m)
    int old_r = a, r = m;
    int old_s = 1, s = 0;
    while (r != 0) {
        int q = old_r / r;
        int tmp = r;   r   = old_r - q * r;   old_r = tmp;
             tmp = s;   s   = old_s - q * s;   old_s = tmp;
    }
    if (old_r != 1)
        throw std::runtime_error("modInverse: gcd != 1, no inverse exists");
    return (old_s % m + m) % m;
}
