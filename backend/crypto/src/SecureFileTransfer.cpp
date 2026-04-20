#include "SecureFileTransfer.h"
#include <stdexcept>

SecureFileTransfer::UploadResult SecureFileTransfer::receiveAndEncrypt(
    const std::string& fileData,
    const std::string& cipher,
    const Config& cfg)
{
    // TODO:
    // 1. Hash::compute(fileData) 得到原始摘要
    // 2. 按 cfg.chunkSize 分片
    // 3. 用 cipher 加密每个分片（AES-256-CTR 等）
    // 4. 每片附 HMAC-SHA1
    // 5. 生成唯一 id，存储分片，返回 UploadResult
    throw std::runtime_error("SecureFileTransfer::receiveAndEncrypt not implemented");
}

SecureFileTransfer::Status SecureFileTransfer::getStatus(const std::string& id) {
    // TODO: 根据 id 查询传输进度
    throw std::runtime_error("SecureFileTransfer::getStatus not implemented");
}

std::string SecureFileTransfer::getFile(const std::string& id) {
    // TODO: 返回解密后的文件数据
    throw std::runtime_error("SecureFileTransfer::getFile not implemented");
}
