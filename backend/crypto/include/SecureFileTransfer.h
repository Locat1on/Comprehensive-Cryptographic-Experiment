#pragma once
#include "Hash.h"
#include "BigInt128.h"
#include <string>

// Forward declaration — avoid pulling in crow_all.h here
namespace crow {
    template<typename... Middlewares>
    class Crow;
    using SimpleApp = Crow<>;
}

class SecureFileTransfer {
public:
    struct Config {
        std::string cipher    = "AES-256-CTR";
        size_t      chunkSize = 4 * 1024 * 1024;
        Hash::Algo  hashAlgo  = Hash::SHA1;
    };

    struct UploadResult {
        std::string id;
        int         chunks;
        std::string hash;
    };

    struct Status {
        std::string id;
        int         progress;   // 0–100
        std::string status;     // "pending" | "transferring" | "done" | "error"
    };

    // 服务端：接收原始文件数据，返回任务 ID
    UploadResult receiveAndEncrypt(const std::string& fileData,
                                   const std::string& cipher,
                                   const Config& cfg);

    // 服务端：查询传输状态
    Status getStatus(const std::string& id);

    // 服务端：返回加密后文件数据
    std::string getFile(const std::string& id);
};
