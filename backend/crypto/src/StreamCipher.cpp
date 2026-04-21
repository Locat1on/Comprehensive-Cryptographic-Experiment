#include "StreamCipher.h"
#include <stdexcept>
#include <fstream>
#include <sstream>

StreamCipher::StreamCipher(const std::vector<uint8_t>& seed, Method method)
    : method_(method), seed_(seed), i_(0), j_(0) {
    if (method == RC4)    initRC4();
    else                  initLFSR();
}

StreamCipher StreamCipher::fromXML(const std::string& xmlPath, Method method) {
    std::ifstream file(xmlPath);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + xmlPath);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());

    auto start = content.find("<seed>");
    auto end   = content.find("</seed>");
    if (start == std::string::npos || end == std::string::npos || end <= start) {
        throw std::runtime_error("Invalid XML format: missing <seed> tag");
    }

    std::string hexSeed = content.substr(start + 6, end - start - 6);

    std::vector<uint8_t> seed;
    std::istringstream iss(hexSeed);
    std::string token;
    while (iss >> token) {
        if (token.empty()) continue;
        seed.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
    }

    if (seed.empty()) {
        throw std::runtime_error("Empty seed in XML file");
    }

    return StreamCipher(seed, method);
}

std::vector<uint8_t> StreamCipher::encrypt(const std::vector<uint8_t>& plaintext) {
    
    if(method_ == RC4) initRC4();
    else initLFSR();

    auto ks = keystream(plaintext.size());

    std::vector<uint8_t> ciphertext(plaintext.size());

    for(size_t i = 0; i < plaintext.size(); i++){
        ciphertext[i] = plaintext[i] ^ ks[i];
    }

    return ciphertext;
}

std::vector<uint8_t> StreamCipher::decrypt(const std::vector<uint8_t>& ciphertext) {
    // XOR 对称，解密与加密相同
    return encrypt(ciphertext);
}

std::vector<uint8_t> StreamCipher::keystream(size_t len) {
    std::vector<uint8_t> out(len);
    
    if(method_ == RC4){
        for(size_t k = 0; k < len; k++){
            out[k] = prga();
        }
    }
    else {
        for(size_t k = 0; k < len; k++){
            uint8_t byte = 0;
            for(int i = 0; i < 8; i++){
                byte = (byte << 1) | lfsrStep();
            }
            out[k] = byte;
        }
    }

    return out;
}

void StreamCipher::initRC4() {
    // KSA — 用 seed_ 初始化 256-byte S 盒
    for(int i = 0; i < 256; i++){
        S_[i] = i;
    }

    int j = 0;
    int keyLen = seed_.size();

    if(keyLen == 0){
        throw std::invalid_argument("seed_ cannot be empty");
    }

    for(int i = 0; i < 256; i++){
        j = (j + S_[i] + seed_[i % keyLen]) % 256;
        std::swap(S_[i], S_[j]);
    }

    i_ = 0;
    j_ = 0;
}

uint8_t StreamCipher::prga(){
    i_ = (i_ + 1) % 256;
    j_ = (j_ + S_[i_]) % 256;

    std::swap(S_[i_], S_[j_]);

    int t = (S_[i_] + S_[j_]) % 256;
    return S_[t];
}

void StreamCipher::initLFSR() {
    if(seed_.empty()){
        S_[0] = 0x01;
    } else{
        S_[0] = seed_[0];
    }

    if(S_[0] == 0){
        S_[0] = 0x01;
    }
}

uint8_t StreamCipher::lfsrStep(){
    uint8_t bit0 = (S_[0] >> 0) & 1;
    uint8_t bit2 = (S_[0] >> 2) & 1;
    uint8_t bit4 = (S_[0] >> 4) & 1;
    uint8_t bit7 = (S_[0] >> 7) & 1;

    uint8_t newBit = bit0 ^ bit2 ^ bit4 ^ bit7;

    uint8_t output = S_[0] & 1;

    S_[0] >>= 1;

    S_[0] = S_[0] | (newBit << 7);

    return output;
}