#pragma once
#include <string>

class Hash {
public:
    enum Algo { SHA1, MD5 };

    static std::string compute    (const std::string& msg,  Algo algo = SHA1);
    static std::string computeFile(const std::string& path, Algo algo = SHA1);
};
