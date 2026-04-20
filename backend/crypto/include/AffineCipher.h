#pragma once
#include <string>

class AffineCipher {
    int a_, b_;
public:
    AffineCipher(int a, int b);
    static AffineCipher fromXML(const std::string& xmlPath);

    std::string encrypt(const std::string& plaintext);
    std::string decrypt(const std::string& ciphertext);
private:
    int modInverse(int a, int m);
};
