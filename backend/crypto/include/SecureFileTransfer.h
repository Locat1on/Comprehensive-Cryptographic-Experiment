#pragma once

#include "Hash.h"

#include <cstdint>
#include <cstddef>
#include <string>

namespace crow {
    template<typename... Middlewares>
    class Crow;
    using SimpleApp = Crow<>;
}

class SecureFileTransfer {
public:
    struct Config {
        std::string cipher    = "SHA1-CTR";
        size_t      chunkSize = 4 * 1024 * 1024;
        Hash::Algo  hashAlgo  = Hash::SHA1;
    };

    struct UploadResult {
        std::string id;
        int         chunks;
        std::string hash;
        std::string signature;
        std::string path;
    };

    struct Status {
        std::string id;
        int         progress;
        std::string status;
    };

    struct StartResult {
        std::string id;
        int         chunkSize;
        int         totalChunks;
        std::string serverPubKey;
        std::string clientPubKey;
        std::string keyFingerprint;
        std::string signature;
    };

    struct ChunkResult {
        std::string id;
        int         receivedChunks;
        int         progress;
        std::string chunkHash;
        std::string encryptedHash;
    };

    UploadResult receiveAndEncrypt(const std::string& fileData,
                                   const std::string& cipher,
                                   const Config& cfg);

    StartResult startUpload(const std::string& fileName,
                            uint64_t fileSize,
                            const std::string& cipher,
                            const Config& cfg);

    ChunkResult receiveChunk(const std::string& id,
                             int chunkIndex,
                             const std::string& chunkData);

    UploadResult finishUpload(const std::string& id);

    Status getStatus(const std::string& id);
    std::string getFile(const std::string& id);
    std::string getFilePath(const std::string& id);
};
