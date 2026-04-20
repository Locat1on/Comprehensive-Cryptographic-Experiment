#pragma once
#include "RSA.h"
#include "Hash.h"
#include <string>

class DigitalSignature {
    RSA rsa_;
public:
    explicit DigitalSignature(const RSA::KeyPair& kp);

    // H = Hash(msg),  S = RSA::sign(H)
    std::string sign  (const std::string& msg, Hash::Algo algo = Hash::SHA1);
    // H' = Hash(msg), H'' = RSA::verify(S) → H'==H''
    bool        verify(const std::string& msg, const std::string& signature,
                       Hash::Algo algo = Hash::SHA1);
};
