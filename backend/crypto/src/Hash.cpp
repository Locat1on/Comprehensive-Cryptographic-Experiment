#include "Hash.h"
#include <stdexcept>

std::string Hash::compute(const std::string& msg, Algo algo) {
    // TODO: algo==SHA1 → SHA-1 实现；algo==MD5 → MD5 实现
    // 返回大写十六进制字符串（SHA1: 40字符，MD5: 32字符）
    throw std::runtime_error("Hash::compute not implemented");
}

std::string Hash::computeFile(const std::string& path, Algo algo) {
    // TODO: 分块读取文件，流式计算哈希
    throw std::runtime_error("Hash::computeFile not implemented");
}
