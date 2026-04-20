#include "DES.h"
#include <stdexcept>

DES::DES(uint64_t key64) {
    keySchedule(key64);
}

DES DES::fromXML(const std::string& xmlPath) {
    // TODO: 解析 <key>HEX</key>
    throw std::runtime_error("DES::fromXML not implemented");
}

uint64_t DES::encryptBlock(uint64_t block) {
    // TODO: IP → 16轮 Feistel → IP⁻¹
    throw std::runtime_error("DES::encryptBlock not implemented");
}

uint64_t DES::decryptBlock(uint64_t block) {
    // TODO: 与加密相同结构，但子密钥逆序使用
    throw std::runtime_error("DES::decryptBlock not implemented");
}

std::vector<uint8_t> DES::encrypt(const std::vector<uint8_t>& data,
                                   Mode mode, uint64_t iv) {
    // TODO: PKCS7 填充 → 按 8 字节分块 → ECB/CBC 加密
    throw std::runtime_error("DES::encrypt not implemented");
}

std::vector<uint8_t> DES::decrypt(const std::vector<uint8_t>& data,
                                   Mode mode, uint64_t iv) {
    // TODO: 按块解密 → 去除 PKCS7 填充
    throw std::runtime_error("DES::decrypt not implemented");
}

uint64_t DES::feistel(uint64_t R, uint64_t K) {
    // TODO: E扩展(48bit) → XOR子密钥 → 8个S盒(32bit) → P置换
    throw std::runtime_error("DES::feistel not implemented");
}

void DES::keySchedule(uint64_t key) {
    // TODO: PC-1置换 → 16轮循环左移 + PC-2 → roundKeys_[0..15]
    throw std::runtime_error("DES::keySchedule not implemented");
}
