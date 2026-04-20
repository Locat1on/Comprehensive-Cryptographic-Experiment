#include "StreamCipher.h"
#include <stdexcept>

StreamCipher::StreamCipher(const std::vector<uint8_t>& seed, Method method)
    : method_(method), seed_(seed), i_(0), j_(0) {
    if (method == RC4)    initRC4();
    else                  initLFSR();
}

StreamCipher StreamCipher::fromXML(const std::string& xmlPath, Method method) {
    // TODO: 解析 <seed>HEX bytes</seed>
    throw std::runtime_error("StreamCipher::fromXML not implemented");
}

std::vector<uint8_t> StreamCipher::encrypt(const std::vector<uint8_t>& plaintext) {
    // TODO: plaintext XOR keystream(plaintext.size())
    throw std::runtime_error("StreamCipher::encrypt not implemented");
}

std::vector<uint8_t> StreamCipher::decrypt(const std::vector<uint8_t>& ciphertext) {
    // XOR 对称，解密与加密相同
    return encrypt(ciphertext);
}

std::vector<uint8_t> StreamCipher::keystream(size_t len) {
    // TODO: 生成 len 字节密钥流
    throw std::runtime_error("StreamCipher::keystream not implemented");
}

void StreamCipher::initRC4() {
    // TODO: KSA — 用 seed_ 初始化 256-byte S 盒
    throw std::runtime_error("StreamCipher::initRC4 not implemented");
}

void StreamCipher::initLFSR() {
    // TODO: f(x)=x⁸+x⁶+x⁴+x+1，taps[0,2,4,7]
    throw std::runtime_error("StreamCipher::initLFSR not implemented");
}
