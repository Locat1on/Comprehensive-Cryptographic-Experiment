# Cryptology — C++ Backend Interface Reference

后端框架：**Crow** (HTTP) | 算法库：自建，无第三方依赖

---

## HTTP 路由总览

| 方法 | 路径 | 模块 |
|------|------|------|
| POST | `/api/v1/affine/encrypt` | 仿射加密 |
| POST | `/api/v1/affine/decrypt` | 仿射解密 |
| POST | `/api/v1/bigint/calc` | 大整数运算 |
| POST | `/api/v1/stream/rc4` | RC4 流密码 |
| POST | `/api/v1/stream/lfsr` | LFSR+JK 流密码 |
| POST | `/api/v1/des/encrypt` | DES 加密 |
| POST | `/api/v1/des/decrypt` | DES 解密 |
| POST | `/api/v1/rsa/keygen` | RSA 密钥生成 |
| POST | `/api/v1/rsa/encrypt` | RSA 加密 |
| POST | `/api/v1/rsa/decrypt` | RSA 解密 |
| POST | `/api/v1/dh/init` | D-H 协议初始化 |
| POST | `/api/v1/dh/exchange` | D-H 密钥交换 |
| POST | `/api/v1/dh/verify` | D-H 签名验证 |
| POST | `/api/v1/hash/compute` | 散列计算 |
| POST | `/api/v1/sign/rsa` | RSA 数字签名 |
| POST | `/api/v1/sign/verify` | 签名验证 |
| POST | `/api/v1/file/encrypt-upload` | 大文件加密上传 |
| GET  | `/api/v1/file/download/:id` | 大文件下载 |
| GET  | `/api/v1/file/status/:id` | 传输状态查询 |

---

## 1. AffineCipher.h

```cpp
// 密钥文件格式: <key><a>7</a><b>3</b></key>
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
```

**请求/响应 (POST /affine/encrypt)**
```json
// Request
{ "plaintext": "HELLO", "a": 7, "b": 3 }
// Response
{ "ciphertext": "XCZZU", "key": { "a": 7, "b": 3 } }
```

---

## 2. BigInt128.h

```cpp
class BigInt128 {
    uint64_t hi_, lo_;
public:
    BigInt128(uint64_t hi = 0, uint64_t lo = 0);
    explicit BigInt128(const std::string& dec);

    BigInt128 operator+(const BigInt128& rhs) const;
    BigInt128 operator-(const BigInt128& rhs) const;
    BigInt256 operator*(const BigInt128& rhs) const;

    bool operator==(const BigInt128&) const;
    bool operator< (const BigInt128&) const;

    std::string toHex() const;   // 32-char uppercase
    std::string toDec() const;   // decimal string
    static BigInt128 fromHex(const std::string&);
};
```

**请求/响应 (POST /bigint/calc)**
```json
// Request
{ "a": "340282366920938463463374607431768211455", "b": "1", "op": "+" }
// Response
{ "result": "340282366920938463463374607431768211456", "hex": "100000000000000000000000000000000" }
```

---

## 3. StreamCipher.h

```cpp
// 种子密钥文件格式: <seed>53 65 63 72 65 74</seed>
class StreamCipher {
public:
    enum Method { RC4, LFSR_JK };

    static StreamCipher fromXML(const std::string& xmlPath, Method m);
    StreamCipher(const std::vector<uint8_t>& seed, Method method = RC4);

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& plaintext);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& ciphertext);
    std::vector<uint8_t> keystream(size_t len);

private:
    // RC4:     KSA (256-byte S-box) + PRGA
    // LFSR_JK: f(x) = x⁸+x⁶+x⁴+x+1, taps[0,2,4,7]
    void initRC4();
    void initLFSR();
};
```

---

## 4. DES.h

```cpp
class DES {
    uint64_t roundKeys_[16];   // 48-bit each
public:
    explicit DES(uint64_t key64);
    static DES fromXML(const std::string& xmlPath);

    uint64_t encryptBlock(uint64_t block);
    uint64_t decryptBlock(uint64_t block);

    std::vector<uint8_t> encrypt(const std::vector<uint8_t>& data,
                                  Mode mode = ECB, uint64_t iv = 0);
    std::vector<uint8_t> decrypt(const std::vector<uint8_t>& data,
                                  Mode mode = ECB, uint64_t iv = 0);
private:
    uint64_t feistel(uint64_t R, uint64_t K);
    void     keySchedule(uint64_t key);
};
```

---

## 5. RSA.h

```cpp
// 模数规模 n < 16-bit (p,q < 256)
// 支持大于 16-bit 的消息分块加密
class RSA {
    BigInt128 n_, e_, d_;
public:
    struct KeyPair { BigInt128 n, e, d; };

    static KeyPair generate(uint16_t p, uint16_t q, uint16_t e);
    explicit RSA(const KeyPair& kp);

    // 分块: 每字节独立 c = m^e mod n
    std::vector<BigInt128> encrypt(const std::string& msg);
    std::string            decrypt(const std::vector<BigInt128>& blocks);

    BigInt128 sign  (const BigInt128& hash);
    bool      verify(const BigInt128& hash, const BigInt128& sig);
};
```

---

## 6. DHProtocol.h

```cpp
class DHProtocol {
    BigInt128 p_, g_;
    BigInt128 privateKey_, publicKey_;
public:
    DHProtocol(BigInt128 p, BigInt128 g);

    BigInt128 generatePublicKey();
    BigInt128 computeSharedKey(const BigInt128& peerPublic);

    // 增强: RSA 签名 + Nonce 防重放
    std::vector<uint8_t> signedExchange(const RSA& signingKey,
                                         const std::string& nonce);
    bool verifyPeer(const std::vector<uint8_t>& signedMsg,
                    const RSA::KeyPair& peerPubKey);
};
```

**协议流程 (C/S 模式)**
```
Client                          Server
  |-- POST /dh/init ----------->|  { p, g, pubKey_C, nonce, sig_C }
  |<-- { pubKey_S, sig_S } -----|
  |-- POST /dh/verify --------->|  { sig_S, hash_msg }
  |<-- { ok: true } ------------|
  |   共享密钥 K = B^a mod p    |  K = A^b mod p
```

---

## 7. Hash.h + DigitalSignature.h

```cpp
class Hash {
public:
    enum Algo { SHA1, MD5 };
    static std::string compute    (const std::string& msg,  Algo algo = SHA1);
    static std::string computeFile(const std::string& path, Algo algo = SHA1);
};

class DigitalSignature {
    RSA rsa_;
public:
    explicit DigitalSignature(const RSA::KeyPair&);

    // sign:   H = Hash(msg),  S = RSA::sign(H)
    std::string sign  (const std::string& msg, Hash::Algo algo = Hash::SHA1);
    // verify: H'= Hash(msg), H''= RSA::verify(S) → H'==H''
    bool        verify(const std::string& msg, const std::string& signature,
                       Hash::Algo algo = Hash::SHA1);
};
```

---

## 8. SecureFileTransfer.h

```cpp
class SecureFileTransfer {
public:
    struct Config {
        std::string cipher   = "AES-256-CTR";
        size_t      chunkSize = 4 * 1024 * 1024;  // 4 MB
        Hash::Algo  hashAlgo  = Hash::SHA1;
    };

    // 发送端 (Client)
    bool sendFile(const std::string& localPath,
                  const std::string& serverAddr,
                  const BigInt128&   sharedKey,   // 来自 DHProtocol
                  const Config&      cfg = {});

    // 接收端 (Server, Crow 路由注册)
    void registerRoutes(crow::SimpleApp& app);

    // 完整性: HMAC-SHA1 每分片校验
    // 来源:   RSA 签名原始文件摘要
    // 续传:   ETag / Range 支持
};
```

**传输流程**
```
1. D-H 协商会话密钥 K
2. SHA-1(file) → 原始摘要 H
3. RSA_Sign(H) → 签名 S
4. 文件按 4MB 分片, AES-256-CTR 加密
5. 每片附 HMAC-SHA1, POST /file/encrypt-upload
6. 接收端逐片验证 HMAC, 解密, 重组
7. 验证 RSA_Verify(S, H') 确认来源
```
