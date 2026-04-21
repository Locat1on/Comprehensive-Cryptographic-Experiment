#include "DES.h"
#include <stdexcept>
#include <fstream>

static const uint8_t PC1[] = {
    57, 49, 41, 33, 25, 17, 9,  1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7,
    56, 48, 40, 32, 24, 16, 8,  0,
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6
};

static const uint8_t PC2[] = {
    13, 16, 10, 23,  0,  4,  2, 27,
    14,  5, 20,  9, 22, 18, 11,  3,
    25,  7, 15,  6, 26, 19, 12,  1,
    40, 51, 30, 36, 46, 54, 29, 39,
    50, 44, 32, 47, 43, 48, 38, 55,
    33, 52, 45, 41, 49, 35, 28, 31
};

static const uint8_t IP[] = {
    58, 50, 42, 34, 26, 18, 10, 2,
    60, 52, 44, 36, 28, 20, 12, 4,
    62, 54, 46, 38, 30, 22, 14, 6,
    64, 56, 48, 40, 32, 24, 16, 8,
    57, 49, 41, 33, 25, 17,  9, 1,
    59, 51, 43, 35, 27, 19, 11, 3,
    61, 53, 45, 37, 29, 21, 13, 5,
    63, 55, 47, 39, 31, 23, 15, 7
};

static const uint8_t FP[] = {
    40, 8, 48, 16, 56, 24, 64, 32,
    39, 7, 47, 15, 55, 23, 63, 31,
    38, 6, 46, 14, 54, 22, 62, 30,
    37, 5, 45, 13, 53, 21, 61, 29,
    36, 4, 44, 12, 52, 20, 60, 28,
    35, 3, 43, 11, 51, 19, 59, 27,
    34, 2, 42, 10, 50, 18, 58, 26,
    33, 1, 41,  9, 49, 17, 57, 25
};

static const uint8_t E[] = {
    32,  1,  2,  3,  4,  5,
     4,  5,  6,  7,  8,  9,
     8,  9, 10, 11, 12, 13,
    12, 13, 14, 15, 16, 17,
    16, 17, 18, 19, 20, 21,
    20, 21, 22, 23, 24, 25,
    24, 25, 26, 27, 28, 29,
    28, 29, 30, 31, 32,  1
};

static const uint8_t P[] = {
    16,  7, 20, 21, 29, 12, 28, 17,
     1, 15, 23, 26,  5, 18, 31, 10,
     2,  8, 24, 14, 32, 27,  3,  9,
    19, 13, 30,  6, 22, 11,  4, 25
};

static const uint8_t S1[] = {
    14,  4, 13,  1,  2, 15, 11,  8,  3, 10,  6, 12,  5,  9,  0,  7,
     0, 15,  7,  4, 14,  2, 13,  1, 10,  6, 12, 11,  9,  5,  3,  8,
     4,  1, 14,  8, 13,  6,  2, 11, 15, 12,  9,  7,  3, 10,  5,  0,
    15, 12,  8,  2,  4,  9,  1,  7,  5, 11,  3, 14, 10,  0,  6, 13
};

static const uint8_t S2[] = {
    15,  1,  8, 14,  6, 11,  3,  4,  9,  7,  2, 13, 12,  0,  5, 10,
     3, 13,  4,  7, 15,  2,  8, 14, 12,  0,  1, 10,  6,  9, 11,  5,
     0, 14,  7, 11, 10,  4, 13,  1,  5,  8, 12,  6,  9,  3,  2, 15,
    13,  8, 10,  1,  3, 15,  4,  2, 11,  6,  7, 12,  0,  5, 14,  9
};

static const uint8_t S3[] = {
    10,  0,  9, 14,  6,  3, 15,  5,  1, 13, 12,  7, 11,  4,  2,  8,
    13,  7,  0,  9,  3,  4,  6, 10,  2,  8,  5, 14, 12, 11, 15,  1,
    13,  6,  4,  9,  8, 15,  3,  0, 11,  1,  2, 12,  5, 10, 14,  7,
     1, 10, 13,  0,  6,  9,  8,  7,  4, 15, 14,  3, 11,  5,  2, 12
};

static const uint8_t S4[] = {
     7, 13, 14,  3,  0,  6,  9, 10,  1,  2,  8,  5, 11, 12,  4, 15,
    13,  8, 11,  5,  6, 15,  0,  3,  4,  7,  2, 12,  1, 10, 14,  9,
    10,  6,  9,  0, 12, 11,  7, 13, 15,  1,  3, 14,  5,  2,  8,  4,
     3, 15,  0,  6, 10,  1, 13,  8,  9,  4,  5, 11, 12,  7,  2, 14
};

static const uint8_t S5[] = {
     2, 12,  4,  1,  7, 10, 11,  6,  8,  5,  3, 15, 13,  0, 14,  9,
    14, 11,  2, 12,  4,  7, 13,  1,  5,  0, 15, 10,  3,  9,  8,  6,
     4,  2,  1, 11, 10, 13,  7,  8, 15,  9, 12,  5,  6,  3,  0, 14,
    11,  8, 12,  7,  1, 14,  2, 13,  6, 15,  0,  9, 10,  4,  5,  3
};

static const uint8_t S6[] = {
    12,  1, 10, 15,  9,  2,  6,  8,  0, 13,  3,  4, 14,  7,  5, 11,
    10, 15,  4,  2,  7, 12,  9,  5,  6,  1, 13, 14,  0, 11,  3,  8,
     9, 14, 15,  5,  2,  8, 12,  3,  7,  0,  4, 10,  1, 13, 11,  6,
     4,  3,  2, 12,  9,  5, 15, 10, 11, 14,  1,  7,  6,  0,  8, 13
};

static const uint8_t S7[] = {
     4, 11,  2, 14, 15,  0,  8, 13,  3, 12,  9,  7,  5, 10,  6,  1,
    13,  0, 11,  7,  4,  9,  1, 10, 14,  3,  5, 12,  2, 15,  8,  6,
     1,  4, 11, 13, 12,  3,  7, 14, 10, 15,  6,  8,  0,  5,  9,  2,
     6, 11, 13,  8,  1,  4, 10,  7,  9,  5,  0, 15, 14,  2,  3, 12
};

static const uint8_t S8[] = {
    13,  2,  8,  4,  6, 15, 11,  1, 10,  9,  3, 14,  5,  0, 12,  7,
     1, 15, 13,  8, 10,  3,  7,  4, 12,  5,  6, 11,  0,  9, 14,  2,
     7, 11,  4,  1,  9, 12, 14,  2,  0,  6, 10, 13, 15,  3,  5,  8,
     2,  1, 14,  7,  4, 10,  8, 13, 15, 12,  9,  0,  3,  5,  6, 11
};

static const uint8_t* SBOXES[] = { S1, S2, S3, S4, S5, S6, S7, S8 };

static const int SHIFTS[] = { 1, 1, 2, 2, 2, 2, 2, 2, 1, 2, 2, 2, 2, 2, 2, 1 };

static uint64_t permute(uint64_t data, const uint8_t* table, int len) {
    uint64_t result = 0;
    for (int i = 0; i < len; i++) {
        int bitPos = table[i] - 1; // Convert 1-based to 0-based
        uint64_t bit = (data >> (63 - bitPos)) & 1;
        result |= (bit << (len - 1 - i));
    }
    return result;
}

static uint32_t rotLeft28(uint32_t val, int shift) {
    shift %= 28;
    return ((val << shift) | (val >> (28 - shift))) & 0x0FFFFFFF;
}

DES::DES(uint64_t key64) {
    keySchedule(key64);
}

DES DES::fromXML(const std::string& xmlPath) {
    std::ifstream file(xmlPath);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open XML file: " + xmlPath);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                        std::istreambuf_iterator<char>());

    size_t keyStart = content.find("<key>");
    size_t keyEnd = content.find("</key>");
    if (keyStart == std::string::npos || keyEnd == std::string::npos) {
        throw std::runtime_error("Invalid XML format: missing <key> tag");
    }

    std::string hexKey = content.substr(keyStart + 6, keyEnd - keyStart - 6);
    uint64_t key = std::stoull(hexKey, nullptr, 16);
    return DES(key);
}

void DES::keySchedule(uint64_t key) {
    uint64_t pc1Result = permute(key, PC1, 56);

    uint32_t C = (uint32_t)(pc1Result >> 28);
    uint32_t D = (uint32_t)(pc1Result & 0x0FFFFFFF);

    for (int i = 0; i < 16; i++) {
        C = rotLeft28(C, SHIFTS[i]);
        D = rotLeft28(D, SHIFTS[i]);

        uint64_t combined = ((uint64_t)C << 28) | D;
        roundKeys_[i] = permute(combined, PC2, 48);
    }
}

uint64_t DES::feistel(uint64_t R, uint64_t K) {
    uint64_t expanded = permute(R, E, 48);
    uint64_t xored = expanded ^ K;

    uint64_t output = 0;
    for (int i = 0; i < 8; i++) {
        uint8_t bits = (uint8_t)((xored >> (42 - i * 6)) & 0x3F);
        uint8_t row = ((bits & 0x20) >> 4) | (bits & 0x01);
        uint8_t col = (bits >> 1) & 0x0F;
        uint8_t sboxVal = SBOXES[i][row * 16 + col];
        output |= ((uint64_t)sboxVal << (28 - i * 4));
    }

    return permute(output, P, 32);
}

uint64_t DES::encryptBlock(uint64_t block) {
    uint64_t ipResult = permute(block, IP, 64);

    uint32_t L = (uint32_t)(ipResult >> 32);
    uint32_t R = (uint32_t)(ipResult & 0xFFFFFFFF);

    for (int i = 0; i < 16; i++) {
        uint32_t newR = L ^ feistel(R, roundKeys_[i]);
        L = R;
        R = newR;
    }

    uint64_t combined = ((uint64_t)R << 32) | L;
    return permute(combined, FP, 64);
}

uint64_t DES::decryptBlock(uint64_t block) {
    uint64_t ipResult = permute(block, IP, 64);

    uint32_t L = (uint32_t)(ipResult >> 32);
    uint32_t R = (uint32_t)(ipResult & 0xFFFFFFFF);

    for (int i = 15; i >= 0; i--) {
        uint32_t newR = L ^ feistel(R, roundKeys_[i]);
        L = R;
        R = newR;
    }

    uint64_t combined = ((uint64_t)R << 32) | L;
    return permute(combined, FP, 64);
}

static std::vector<uint8_t> pkcs7Pad(const std::vector<uint8_t>& data) {
    size_t padLen = 8 - (data.size() % 8);
    std::vector<uint8_t> padded = data;
    for (size_t i = 0; i < padLen; i++) {
        padded.push_back(static_cast<uint8_t>(padLen));
    }
    return padded;
}

static std::vector<uint8_t> pkcs7Unpad(const std::vector<uint8_t>& data) {
    if (data.empty()) {
        throw std::runtime_error("Invalid padding: empty data");
    }

    uint8_t padLen = data.back();
    if (padLen == 0 || padLen > 8 || data.size() < padLen) {
        throw std::runtime_error("Invalid padding length");
    }

    for (uint8_t i = 0; i < padLen; i++) {
        if (data[data.size() - 1 - i] != padLen) {
            throw std::runtime_error("Invalid padding content");
        }
    }

    return std::vector<uint8_t>(data.begin(), data.end() - padLen);
}

static uint64_t bytesToBlock(const uint8_t* data) {
    uint64_t block = 0;
    for (int i = 0; i < 8; i++) {
        block = (block << 8) | data[i];
    }
    return block;
}

static void blockToBytes(uint64_t block, uint8_t* data) {
    for (int i = 7; i >= 0; i--) {
        data[i] = static_cast<uint8_t>(block & 0xFF);
        block >>= 8;
    }
}

std::vector<uint8_t> DES::encrypt(const std::vector<uint8_t>& data,
                                   Mode mode, uint64_t iv) {
    std::vector<uint8_t> padded = pkcs7Pad(data);
    std::vector<uint8_t> result;

    uint64_t prevBlock = iv;

    for (size_t i = 0; i < padded.size(); i += 8) {
        uint64_t block = bytesToBlock(&padded[i]);

        if (mode == CBC) {
            block ^= prevBlock;
        }

        uint64_t encrypted = encryptBlock(block);
        prevBlock = encrypted;

        uint8_t bytes[8];
        blockToBytes(encrypted, bytes);
        for (int j = 0; j < 8; j++) {
            result.push_back(bytes[j]);
        }
    }

    return result;
}

std::vector<uint8_t> DES::decrypt(const std::vector<uint8_t>& data,
                                   Mode mode, uint64_t iv) {
    if (data.size() % 8 != 0) {
        throw std::runtime_error("Invalid ciphertext length");
    }

    std::vector<uint8_t> result;
    uint64_t prevBlock = iv;

    for (size_t i = 0; i < data.size(); i += 8) {
        uint64_t block = bytesToBlock(&data[i]);
        uint64_t decrypted = decryptBlock(block);

        if (mode == CBC) {
            decrypted ^= prevBlock;
        }
        prevBlock = block;

        uint8_t bytes[8];
        blockToBytes(decrypted, bytes);
        for (int j = 0; j < 8; j++) {
            result.push_back(bytes[j]);
        }
    }

    return pkcs7Unpad(result);
}
