#pragma once
#include <vector>
#include <cstdint>
#include <string>

class DES {
public:
    enum Mode { ECB, CBC };

    explicit DES(uint64_t key64);
    static DES fromXML(const std::string& xmlPath);

    uint64_t encryptBlock(uint64_t block);
    uint64_t decryptBlock(uint64_t block);

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                                  Mode mode = ECB, uint64_t iv = 0);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data,
                                  Mode mode = ECB, uint64_t iv = 0);
private:
    uint64_t roundKeys_[16];

    uint64_t feistel(uint64_t R, uint64_t K);
    void     keySchedule(uint64_t key);
};
