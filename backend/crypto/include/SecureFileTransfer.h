#pragma once

#include "Hash.h"

#include <cstdint>
#include <cstddef>
#include <string>
#include <vector>

namespace crow {
    template<typename... Middlewares>
    class Crow;
    using SimpleApp = Crow<>;
}

class SecureFileTransfer {
public:
    struct Config {
        std::string cipher    = "E2E-XOR";
        size_t      chunkSize = 4 * 1024 * 1024;
    };

    struct StartResult {
        std::string id;
        int         chunkSize;
        int         totalChunks;
        std::string status;
    };

    struct JoinResult {
        std::string senderPubKey;
        std::string fileName;
        uint64_t    fileSize;
        int         totalChunks;
        int         chunkSize;
    };

    struct ChunkResult {
        std::string id;
        int         receivedChunks;
        int         progress;
        std::string encryptedHash;
    };

    struct TransferInfo {
        std::string id;
        std::string fileName;
        uint64_t    fileSize;
        int         totalChunks;
        int         receivedChunks;
        std::string status;
        std::string senderPubKey;
        std::string receiverPubKey;
        std::string cipher;
    };

    StartResult startUpload(const std::string& fileName,
                            uint64_t fileSize,
                            const std::string& senderPubKey,
                            const Config& cfg);

    JoinResult joinTransfer(const std::string& id, const std::string& receiverPubKey);

    ChunkResult receiveChunk(const std::string& id,
                             int chunkIndex,
                             const std::string& encryptedData);

    std::string getChunk(const std::string& id, int chunkIndex);

    void finishUpload(const std::string& id);

    TransferInfo getTransferInfo(const std::string& id);
    std::vector<TransferInfo> listTransfers();
};
