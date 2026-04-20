# Cryptology — C++ Backend Interface Reference

后端框架：**Crow** (HTTP) | 算法库：自建 | 依赖：**ASIO** (header-only)

---

## 构建说明

### 环境要求

| 组件 | 要求 | 说明 |
|------|------|------|
| 编译器 | g++ ≥ 11 或 clang++ ≥ 12 | 需支持 C++17 |
| ASIO | ≥ 1.30 | header-only，无需编译 |
| 平台 | Windows (PowerShell) | 当前仅支持 Windows |

### 快速构建

```powershell
cd backend
.\build.ps1
```

### 编译器配置

脚本自动检测以下路径（按优先级）：
1. 环境变量 `$env:GXX` / `$env:AR`
2. PATH 中的 `g++` / `ar`
3. 常见安装路径（MSYS2、MinGW-w64）

```powershell
# 手动指定编译器
$env:GXX = "C:\msys64\mingw64\bin\g++.exe"
$env:AR = "C:\msys64\mingw64\bin\ar.exe"
.\build.ps1
```

### ASIO 配置

```powershell
# 如果 ASIO 不在标准路径
$env:ASIO_ROOT = "C:\asio-1.30.2\include"
.\build.ps1
```

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
// Forward declaration — avoid pulling in crow_all.h here
namespace crow {
    template<typename... Middlewares>
    class Crow;
    using SimpleApp = Crow<>;
}

class SecureFileTransfer {
public:
    struct Config {
        std::string cipher    = "AES-256-CTR";
        size_t      chunkSize = 4 * 1024 * 1024;
        Hash::Algo  hashAlgo  = Hash::SHA1;
    };

    struct UploadResult {
        std::string id;
        int         chunks;
        std::string hash;
    };

    struct Status {
        std::string id;
        int         progress;   // 0–100
        std::string status;     // "pending" | "transferring" | "done" | "error"
    };

    // 服务端：接收原始文件数据，返回任务 ID
    UploadResult receiveAndEncrypt(const std::string& fileData,
                                   const std::string& cipher,
                                   const Config& cfg);

    // 服务端：查询传输状态
    Status getStatus(const std::string& id);

    // 服务端：返回加密后文件数据
    std::string getFile(const std::string& id);
};
```

**注意：** `crow::SimpleApp` 是模板类 `Crow<>` 的别名，前向声明需使用模板语法。

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

---

## 添加新模块指南

### 1. 创建头文件

在 `backend/crypto/include/` 创建 `YourModule.h`：

```cpp
#pragma once
#include <string>

class YourModule {
public:
    struct Config {
        // 默认配置参数
    };
    
    struct Result {
        // 返回结果结构
    };
    
    // 构造函数
    explicit YourModule(const Config& cfg);
    
    // 主要方法
    Result process(const std::string& input);
    
private:
    // 内部辅助方法
    void helper();
};
```

### 2. 创建实现文件

在 `backend/crypto/src/` 创建 `YourModule.cpp`：

```cpp
#include "YourModule.h"
#include <stdexcept>

YourModule::YourModule(const Config& cfg) {
    // TODO: 初始化
}

YourModule::Result YourModule::process(const std::string& input) {
    // TODO: 实现算法
    throw std::runtime_error("NOT_IMPLEMENTED");
}
```

### 3. 添加 HTTP 路由

在 `backend/src/main.cpp` 中添加：

```cpp
#include "YourModule.h"

// 在 main() 中添加路由
CROW_ROUTE(app, "/api/v1/yourmodule/action")
.methods("POST"_method)
([](const crow::request& req) {
    auto body = crow::json::load(req.body);
    if (!body) return err(400, "invalid JSON");
    
    // 解析参数
    std::string input = body["input"].s();
    
    // 调用算法
    YourModule::Config cfg;
    YourModule module(cfg);
    auto result = module.process(input);
    
    // 返回结果
    return ok(crow::json::wvalue{
        {"output", result.output}
    });
});
```

### 4. 更新构建脚本

在 `backend/build.ps1` 的 `$sourceFiles` 数组中添加：

```powershell
@{ src = "crypto\src\YourModule.cpp"; obj = "crypto\YourModule.o" },
```

### 5. 前端集成

在 `frontend/js/panels/` 创建 `yourmodule.jsx`，参考现有面板实现。

### 接口设计规范

| 项目 | 规范 |
|------|------|
| 类名 | PascalCase，如 `AESCipher` |
| 方法名 | camelCase，如 `encryptBlock` |
| 参数 | 使用 `const std::string&` 传递大字符串 |
| 返回值 | 复杂结果使用结构体，简单值直接返回 |
| 错误处理 | 抛出 `std::runtime_error`，描述错误原因 |
| 默认参数 | 使用 `Config` 结构体封装，避免过长参数列表 |

### 前向声明 Crow 类型

如果需要在头文件中使用 Crow 类型但不包含 `crow_all.h`：

```cpp
namespace crow {
    template<typename... Middlewares>
    class Crow;
    using SimpleApp = Crow<>;
}
```
