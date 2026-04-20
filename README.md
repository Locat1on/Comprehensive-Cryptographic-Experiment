# Comprehensive Cryptographic Experiment

加解密综合服务平台 — 多文件 React 前端 + C++ Crow 后端 + 算法静态库。

---

## 项目结构

```
Comprehensive-Cryptographic-Experiment/
├── CppInterface.md              C++ 类定义与接口详细说明
├── README.md                    本文件
├── Cryptology.html              旧版单文件前端（已弃用，仅作备份）
│
├── frontend/                    前端（多文件 React，无任何计算逻辑）
│   ├── index.html               入口页面
│   ├── css/
│   │   └── styles.css           全部样式
│   └── js/
│       ├── api.js               后端请求封装（apiCall / apiGet / apiUpload）
│       ├── utils.js             工具函数（copyText / timestamp）
│       ├── components.jsx       公共组件（ResultBox / FileDropZone 等）
│       ├── app.jsx              主应用与侧边栏导航
│       └── panels/              各功能面板（每个模块独立文件）
│           ├── affine.jsx       仿射加密
│           ├── bigint.jsx       大整数运算
│           ├── stream.jsx       流密码
│           ├── des.jsx          DES 对称加密
│           ├── rsa.jsx          RSA 非对称加密
│           ├── dh.jsx           D-H 密钥交换
│           ├── hash.jsx         哈希 + 数字签名
│           └── file.jsx         大文件加密传输
│
└── backend/                     后端
    ├── build.bat                一键构建脚本（CMD）
    ├── build.ps1                一键构建脚本（PowerShell，推荐）
    ├── server.exe               编译产物
    ├── crypto/                  算法库
    │   ├── include/             头文件（类声明，接口已定义）
    │   │   ├── AffineCipher.h
    │   │   ├── BigInt128.h
    │   │   ├── StreamCipher.h
    │   │   ├── DES.h
    │   │   ├── RSA.h
    │   │   ├── DHProtocol.h
    │   │   ├── Hash.h
    │   │   ├── DigitalSignature.h
    │   │   └── SecureFileTransfer.h
    │   └── src/                 实现文件（待填写 TODO）
    │       ├── AffineCipher.cpp
    │       ├── BigInt128.cpp
    │       ├── StreamCipher.cpp
    │       ├── DES.cpp
    │       ├── RSA.cpp
    │       ├── DHProtocol.cpp
    │       ├── Hash.cpp
    │       ├── DigitalSignature.cpp
    │       └── SecureFileTransfer.cpp
    ├── lib/
    │   ├── crow_all.h           Crow HTTP 框架（单头文件 v1.2.0）
    │   └── libcrypto.a          算法静态库（编译产物）
    └── src/
        └── main.cpp             Crow 服务器（路由 + 静态文件托管）
```

---

## 架构说明

```
┌─────────────────────────────────────────────────────────┐
│  浏览器  http://localhost:8080/                          │
│  frontend/index.html + css/ + js/                       │
│  前端只负责 UI 与 API 调用，不含任何密码学计算           │
└────────────────────────┬────────────────────────────────┘
                         │ fetch (JSON / FormData)
                         ▼
┌─────────────────────────────────────────────────────────┐
│  Crow HTTP Server  :8080                                │
│  src/main.cpp                                           │
│  ├── GET  /             托管 frontend/index.html        │
│  ├── GET  /<path>       托管 frontend/ 静态资源          │
│  └── POST /api/v1/...   调用 libcrypto.a 中的算法        │
└────────────────────────┬────────────────────────────────┘
                         │ 函数调用
                         ▼
┌─────────────────────────────────────────────────────────┐
│  lib/libcrypto.a   算法静态库                            │
│  AffineCipher │ BigInt128 │ StreamCipher │ DES          │
│  RSA │ DHProtocol │ Hash │ DigitalSignature             │
│  SecureFileTransfer                                     │
└─────────────────────────────────────────────────────────┘
```

**职责划分：**

| 层 | 位置 | 职责 |
|----|------|------|
| 前端展示层 | `frontend/` | UI 渲染、用户输入、调用后端接口、展示结果 |
| HTTP 路由层 | `src/main.cpp` | 解析请求、调用算法、返回 JSON、托管静态文件 |
| 算法实现层 | `crypto/` | 所有密码学计算，编译为 `libcrypto.a` |

---

## 快速上手

### 环境要求

| 工具 | 说明 |
|------|------|
| g++ ≥ 11 | 无 |
| asio ≥ 1.30 | MSYS2 安装：`pacman -S mingw-w64-x86_64-asio` |
| ar | 随 g++ 附带，用于打包静态库 |

### 构建

在 `backend/` 目录下运行：

**PowerShell (推荐):**
```powershell
.\build.ps1
```

构建分三步：
1. `crypto/src/*.cpp` → `.o` 目标文件
2. `ar rcs lib/libcrypto.a *.o` → 算法静态库
3. `src/main.cpp` + `libcrypto.a` → `server.exe`

#### 配置编译器路径

脚本会按以下优先级检测编译器：
1. 环境变量 `GXX` / `AR`
2. PATH 中的 `g++` / `ar`
3. 默认 MSYS2 路径

**临时指定编译器路径：**
```powershell
$env:GXX = "C:\msys64\mingw64\bin\g++.exe"
$env:AR = "C:\msys64\mingw64\bin\ar.exe"
.\build.ps1
```

**手动编译命令：**
```cmd
g++ -std=c++17 -O2 -Ilib -Icrypto\include -DASIO_STANDALONE ^
  src\main.cpp -Llib -lcrypto -o server.exe -lws2_32 -lmswsock
```

### 启动

```cmd
cd backend
server.exe
```

看到以下输出即启动成功：

```
[INFO] Frontend: http://localhost:8080/
[INFO] Crow/1.2.0 server is running at http://0.0.0.0:8080
```

浏览器打开 **http://localhost:8080/** 即可使用前端界面。

> 注意：不要直接双击打开 `frontend/index.html`，Babel 在 `file://` 协议下无法加载外部 `.jsx` 文件，必须通过 HTTP 访问。

### 修改后端地址

如需部署到其他地址，修改 `frontend/js/api.js` 第一行：

```js
const API_BASE = 'http://localhost:8080/api/v1';
```

---

## 新成员上手流程

### 1. 环境准备

```powershell
# 克隆仓库
git clone <仓库地址>
cd Comprehensive-Cryptographic-Experiment

# 确认编译器可用
g++ --version  # 需支持 C++17
```

### 2. 首次构建验证

```powershell
cd backend
.\build.ps1
.\server.exe
```

浏览器访问 http://localhost:8080/，确认页面正常加载（此时所有功能返回 "NOT_IMPLEMENTED" 是正常的）。

### 3. 选择首个任务

建议按以下顺序完成：

| 顺序 | 模块 | 难度 | 说明 |
|------|------|------|------|
| 1 | AffineCipher | ⭐ 简单 | 古典密码，无依赖，适合熟悉项目结构 |
| 2 | DES | ⭐⭐ 中等 | 标准算法，资料丰富，前端已就绪 |
| 3 | Hash | ⭐⭐ 中等 | SHA1/MD5，为数字签名打基础 |
| 4 | BigInt128 | ⭐⭐⭐ 较难 | 大整数运算，RSA 的基础依赖 |
| 5 | RSA | ⭐⭐⭐ 较难 | 依赖 BigInt128，非对称加密核心 |

### 4. 任务完成 checklist

- [ ] 算法实现通过本地测试（至少 2 组用例）
- [ ] `build.ps1` 构建无警告、无错误
- [ ] 浏览器联调功能正常
- [ ] 代码注释清晰，关键步骤有说明
- [ ] 提交信息规范：`feat(DES): 实现 ECB 和 CBC 模式加密解密`

---

## 算法实现指南

所有算法实现文件在 `backend/crypto/src/`，每个方法内均有 `// TODO:` 注释。

### 实现步骤

1. **选择模块**：根据「实现优先级」表格，建议从 P0 级别开始
2. **阅读接口**：查看 `crypto/include/` 对应 `.h` 头文件，了解类定义和方法签名
3. **实现算法**：在 `crypto/src/` 对应 `.cpp` 文件中替换 `throw std::runtime_error(...)`，填写实际算法
4. **本地测试**：在 `.cpp` 底部添加临时 `main()` 函数进行单元测试（提交前删除）
5. **构建运行**：在 `backend/` 目录运行 `build.ps1` 重新构建，启动 `server.exe`
6. **联调验证**：浏览器打开 http://localhost:8080/ 测试对应功能面板
7. **提交代码**：确保无编译警告，提交信息格式：`feat(模块): 实现功能简述`

### 开发规范

| 项目 | 规范 |
|------|------|
| 代码风格 | 遵循现有文件缩进（4空格），保持与周边代码一致 |
| 命名规范 | 类名 PascalCase，方法名 camelCase，常量全大写下划线分隔 |
| 错误处理 | 使用异常 `throw std::runtime_error("描述")` 报告错误 |
| 输入验证 | 所有公开方法需验证参数合法性（如密钥范围、空指针等） |
| 注释要求 | 复杂算法步骤添加注释，引用算法出处（如教材章节） |
| 测试数据 | 提供至少 2 组测试用例（正常输入 + 边界情况） |

### 调试技巧

```cpp
// 在算法实现中添加调试输出（提交前删除）
#include <iostream>
std::cerr << "[Debug] 变量值: " << value << std::endl;
```

### 类依赖关系

```
BigInt128
  ├── RSA（存储 n/e/d）
  │     ├── DHProtocol（RSA 签名）
  │     └── DigitalSignature（RSA 签名）
  └── SecureFileTransfer（传递共享密钥）

Hash
  └── DigitalSignature（先哈希再签名）

AffineCipher   （独立）
StreamCipher   （独立）
DES            （独立）
```

> **注意：** `RSA`、`DHProtocol`、`DigitalSignature` 均依赖 `BigInt128`，请优先完成 `BigInt128.cpp`。

### 实现优先级

| 优先级 | 模块 | 文件 | 前端状态 |
|--------|------|------|----------|
| P0 | BigInt128 | `BigInt128.cpp` | 多个模块的基础依赖 |
| P0 | DES | `DES.cpp` | 前端已接入，实现后立即可用 |
| P0 | Hash | `Hash.cpp` | 前端已接入，实现后立即可用 |
| P0 | DigitalSignature | `DigitalSignature.cpp` | 前端已接入，实现后立即可用 |
| P1 | RSA | `RSA.cpp` | 前端已接入，依赖 BigInt128 |
| P1 | D-H | `DHProtocol.cpp` | 前端已接入，依赖 RSA |
| P1 | SecureFileTransfer | `SecureFileTransfer.cpp` | 前端已接入 |
| P2 | AffineCipher | `AffineCipher.cpp` | 前端已接入 |
| P2 | StreamCipher | `StreamCipher.cpp` | 前端已接入 |

---

## 后端 API 接口

> 所有响应均包含 CORS 头，支持浏览器跨域访问。
> 后端未实现时返回 `"NOT_IMPLEMENTED"`，前端会原样显示。

### 1. 仿射加密

| 方法 | 路径 |
|------|------|
| POST | `/api/v1/affine/encrypt` |
| POST | `/api/v1/affine/decrypt` |

```json
// 加密请求
{ "plaintext": "HELLO", "a": 7, "b": 3 }
// 加密响应
{ "ciphertext": "XCZZU", "key": { "a": 7, "b": 3 } }

// 解密请求
{ "ciphertext": "XCZZU", "a": 7, "b": 3 }
// 解密响应
{ "plaintext": "HELLO" }
```

### 2. 大整数运算

| 方法 | 路径 |
|------|------|
| POST | `/api/v1/bigint/calc` |

```json
// 请求（op: "+" / "-" / "*"）
{ "a": "340282366920938463463374607431768211455", "b": "1", "op": "+" }
// 响应
{ "result": "340282366920938463463374607431768211456", "hex": "100000000000000000000000000000000" }
```

### 3. 流密码

| 方法 | 路径 |
|------|------|
| POST | `/api/v1/stream/rc4` |
| POST | `/api/v1/stream/lfsr` |

```json
// 请求（seed 为空格分隔 HEX 字节串）
{ "seed": "53 65 63 72 65 74 4B 65 79", "plaintext": "Hello, Cryptology!" }
// 响应
{ "ciphertext": "A1 B2 ...", "keystream": "F3 C4 ..." }
```

### 4. DES 对称加密

| 方法 | 路径 |
|------|------|
| POST | `/api/v1/des/encrypt` |
| POST | `/api/v1/des/decrypt` |

```json
// 请求（mode: "ECB" / "CBC"，CBC 时可附加 iv 字段）
{ "data": "0123456789ABCDEF", "key": "133457799BBCDFF1", "mode": "ECB" }
// 响应
{ "result": "85E813540F0AB405" }
```

### 5. RSA 非对称加密

| 方法 | 路径 |
|------|------|
| POST | `/api/v1/rsa/keygen` |
| POST | `/api/v1/rsa/encrypt` |
| POST | `/api/v1/rsa/decrypt` |

```json
// 密钥生成请求
{ "p": 61, "q": 53, "e": 17 }
// 密钥生成响应
{ "n": 3233, "e": 17, "d": 2753, "phi": 3120 }

// 加密请求
{ "message": "Hello RSA", "key": { "n": 3233, "e": 17 } }
// 加密响应
{ "blocks": [2790, 1655, 2844, 2790, 2762] }

// 解密请求
{ "blocks": [2790, 1655, 2844, 2790, 2762], "key": { "n": 3233, "d": 2753 } }
// 解密响应
{ "message": "Hello RSA" }
```

### 6. D-H 密钥交换

| 方法 | 路径 |
|------|------|
| POST | `/api/v1/dh/init` |
| POST | `/api/v1/dh/exchange` |
| POST | `/api/v1/dh/verify` |

```json
// 初始化请求
{ "p": "23", "g": "5" }
// 初始化响应（服务端生成公钥并签名）
{ "pubKey": "19", "signature": "..." }

// 交换请求
{ "pubKey": "19", "p": "23", "g": "5" }
// 交换响应
{ "sharedKey": "..." }

// 验证请求
{ "signature": "...", "hash": "..." }
// 验证响应
{ "ok": true }
```

### 7. 哈希 + 数字签名

| 方法 | 路径 |
|------|------|
| POST | `/api/v1/hash/compute` |
| POST | `/api/v1/sign/rsa` |
| POST | `/api/v1/sign/verify` |

```json
// 哈希请求（algo: "SHA1" / "MD5"）
{ "message": "Verify this message", "algo": "SHA1" }
// 哈希响应
{ "hash": "DA39A3EE5E6B4B0D3255BFEF95601890AFD80709" }

// 签名请求
{ "message": "Verify this message", "algo": "SHA1" }
// 签名响应
{ "signature": "A1B2C3...", "hash": "DA39A3EE..." }

// 验证请求
{ "message": "Verify this message", "signature": "A1B2C3...", "algo": "SHA1" }
// 验证响应
{ "valid": true, "hash": "DA39A3EE..." }
```

### 8. 大文件加密传输

| 方法 | 路径 |
|------|------|
| POST | `/api/v1/file/encrypt-upload` |
| GET  | `/api/v1/file/status/:id` |
| GET  | `/api/v1/file/download/:id` |

```
// 上传请求：multipart/form-data
// 字段：file(binary)、cipher("AES-256-CTR"/"AES-256-CBC"/"ChaCha20-Poly1305")、chunk_size(默认4194304)
// 上传响应
{ "id": "tx_abc123", "chunks": 12, "hash": "DA39A3EE..." }

// 状态响应（status: "pending" / "transferring" / "done" / "error"）
{ "id": "tx_abc123", "progress": 65, "status": "transferring" }

// 下载响应：Content-Type: application/octet-stream
```
