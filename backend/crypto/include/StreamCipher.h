#pragma once
#include <vector>
#include <cstdint>
#include <string>
#include <algorithm>

class StreamCipher {
public:
    enum Method { RC4, LFSR_JK };

    StreamCipher(const std::vector<uint8_t>& seed, Method method = RC4);
    static StreamCipher fromXML(const std::string& xmlPath, Method method);

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext);
    std::vector<uint8_t> keystream(size_t len);

private:
    Method method_;
    std::vector<uint8_t> seed_;
    uint8_t S_[256];
    int i_, j_;

    uint8_t prga();
    uint8_t lfsrStep();
    void initRC4();
    void initLFSR();
};

