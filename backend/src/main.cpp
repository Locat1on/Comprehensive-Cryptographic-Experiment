#define CROW_MAIN
#include "crow_all.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <mutex>
#include <random>
#include <ctime>

#include "AffineCipher.h"
#include "BigInt128.h"
#include "StreamCipher.h"
#include "DES.h"
#include "RSA.h"
#include "DHProtocol.h"
#include "Hash.h"
#include "DigitalSignature.h"
#include "SecureFileTransfer.h"

// ── 前端静态文件目录（相对于 server.exe 所在目录）────────────
static const std::string FRONTEND_DIR = "../frontend/";

static std::string read_file(const std::string& path) {
    std::ifstream f(path, std::ios::binary);
    if (!f) return "";
    std::ostringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

static std::string mime_type(const std::string& path) {
    static const std::unordered_map<std::string, std::string> types = {
        {".html", "text/html; charset=utf-8"},
        {".css",  "text/css; charset=utf-8"},
        {".js",   "application/javascript; charset=utf-8"},
        {".jsx",  "application/javascript; charset=utf-8"},
        {".png",  "image/png"},
        {".ico",  "image/x-icon"},
    };
    auto dot = path.rfind('.');
    if (dot != std::string::npos) {
        auto it = types.find(path.substr(dot));
        if (it != types.end()) return it->second;
    }
    return "application/octet-stream";
}

// ── CORS helper ──────────────────────────────────────────────
static void set_cors(crow::response& res) {
    res.set_header("Access-Control-Allow-Origin",  "*");
    res.set_header("Access-Control-Allow-Headers", "Content-Type");
    res.set_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
}

// 构造 JSON 错误响应
static crow::response err(int code, const std::string& msg) {
    crow::response res(code, crow::json::wvalue{{"error", msg}}.dump());
    res.set_header("Content-Type", "application/json");
    set_cors(res);
    return res;
}

// 构造 JSON 成功响应
static crow::response ok(crow::json::wvalue body) {
    crow::response res(200, body.dump());
    res.set_header("Content-Type", "application/json");
    set_cors(res);
    return res;
}

// ── HEX / bytes 转换工具 ──────────────────────────────────
static std::vector<uint8_t> hex_to_bytes(const std::string& hex) {
    std::vector<uint8_t> out;
    std::istringstream iss(hex);
    std::string token;
    while (iss >> token) {
        if (token.empty()) continue;
        out.push_back(static_cast<uint8_t>(std::stoul(token, nullptr, 16)));
    }
    return out;
}

static std::string bytes_to_hex(const std::vector<uint8_t>& bytes) {
    std::ostringstream oss;
    for (size_t i = 0; i < bytes.size(); i++) {
        if (i > 0) oss << ' ';
        oss << std::uppercase << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(bytes[i]);
    }
    return oss.str();
}

static std::vector<uint8_t> string_to_bytes(const std::string& s) {
    return std::vector<uint8_t>(s.begin(), s.end());
}

static Hash::Algo parse_hash_algo(const std::string& algo) {
    return algo == "MD5" ? Hash::MD5 : Hash::SHA1;
}

static RSA::KeyPair demo_signing_key() {
    return RSA::generate(61, 53, 17);
}

static BigInt128 digest_to_bigint(const std::string& digest) {
    std::string h;
    for (unsigned char c : digest) {
        if (!std::isspace(c)) h.push_back(static_cast<char>(std::toupper(c)));
    }
    if (h.size() > 32) h = h.substr(h.size() - 32);
    return BigInt128::fromHex(h);
}

static uint64_t mod_pow_u64(uint64_t base, uint64_t exp, uint64_t mod) {
    uint64_t result = 1 % mod;
    base %= mod;
    while (exp > 0) {
        if (exp & 1ULL) result = static_cast<uint64_t>((unsigned __int128)result * base % mod);
        base = static_cast<uint64_t>((unsigned __int128)base * base % mod);
        exp >>= 1U;
    }
    return result;
}

struct DHServerSession {
    uint64_t p = 0;
    uint64_t g = 0;
    uint64_t clientPub = 0;
    uint64_t serverPriv = 0;
    uint64_t serverPub = 0;
    std::string nonce;
    std::string message;
    std::string signature;
};

static std::mutex g_dh_mutex;
static std::unordered_map<std::string, DHServerSession> g_dh_sessions;

static std::string make_session_id(const std::string& prefix) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<uint32_t> dist;
    std::ostringstream oss;
    oss << prefix << std::hex << std::time(nullptr) << "_" << dist(gen);
    return oss.str();
}

int main() {
    crow::SimpleApp app;

    // ── OPTIONS 预检（全局通配）────────────────────────────
    CROW_ROUTE(app, "/<path>")
    .methods("OPTIONS"_method)
    ([](const crow::request&, const std::string&) {
        crow::response res(204);
        set_cors(res);
        return res;
    });

    // ── 1. 仿射加密 ───────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/affine/encrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string plaintext = body["plaintext"].s();
        int a = body["a"].i();
        int b = body["b"].i();

        try {
            AffineCipher cipher(a, b);
            return ok(crow::json::wvalue{
                {"ciphertext", cipher.encrypt(plaintext)},
                {"key", crow::json::wvalue{{"a", a}, {"b", b}}}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/affine/decrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string ciphertext = body["ciphertext"].s();
        int a = body["a"].i();
        int b = body["b"].i();

        try {
            AffineCipher cipher(a, b);
            return ok(crow::json::wvalue{{"plaintext", cipher.decrypt(ciphertext)}});
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    // ── 2. 大整数运算 ─────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/bigint/calc")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string a  = body["a"].s();
        std::string b  = body["b"].s();
        std::string op = body["op"].s();

        try {
            BigInt128 ba(a), bb(b);
            if (op == "+") {
                BigInt128 r = ba + bb;
                return ok(crow::json::wvalue{{"result", r.toDec()}, {"hex", r.toHex()}});
            } else if (op == "-") {
                BigInt128 r = ba - bb;
                return ok(crow::json::wvalue{{"result", r.toDec()}, {"hex", r.toHex()}});
            } else if (op == "*") {
                BigInt256 r = ba * bb;
                return ok(crow::json::wvalue{{"result", r.toDec()}, {"hex", r.toHex()}});
            } else {
                return err(400, "unknown op: " + op);
            }
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    // ── 3. 流密码 ─────────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/stream/rc4/encrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string seed      = body["seed"].s();
        std::string plaintext = body["plaintext"].s();

        auto seed_bytes      = hex_to_bytes(seed);
        auto plaintext_bytes = string_to_bytes(plaintext);

        if (seed_bytes.empty()) return err(400, "invalid seed");

        StreamCipher sc(seed_bytes, StreamCipher::RC4);
        auto cipher = sc.encrypt(plaintext_bytes);
        auto ks     = sc.keystream(plaintext_bytes.size());

        return ok(crow::json::wvalue{
            {"ciphertext", bytes_to_hex(cipher)},
            {"keystream",  bytes_to_hex(ks)}
        });
    });

    CROW_ROUTE(app, "/api/v1/stream/lfsr/encrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string seed      = body["seed"].s();
        std::string plaintext = body["plaintext"].s();

        auto seed_bytes      = hex_to_bytes(seed);
        auto plaintext_bytes = string_to_bytes(plaintext);

        if (seed_bytes.empty()) return err(400, "invalid seed");

        StreamCipher sc(seed_bytes, StreamCipher::LFSR_JK);
        auto cipher = sc.encrypt(plaintext_bytes);
        auto ks     = sc.keystream(plaintext_bytes.size());

        return ok(crow::json::wvalue{
            {"ciphertext", bytes_to_hex(cipher)},
            {"keystream",  bytes_to_hex(ks)}
        });
    });

    CROW_ROUTE(app, "/api/v1/stream/rc4/decrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string seed       = body["seed"].s();
        std::string ciphertext = body["ciphertext"].s();

        auto seed_bytes   = hex_to_bytes(seed);
        auto cipher_bytes = hex_to_bytes(ciphertext);

        if (seed_bytes.empty())   return err(400, "invalid seed");
        if (cipher_bytes.empty()) return err(400, "invalid ciphertext");

        StreamCipher sc(seed_bytes, StreamCipher::RC4);
        auto plain = sc.decrypt(cipher_bytes);

        return ok(crow::json::wvalue{
            {"plaintext", std::string(plain.begin(), plain.end())}
        });
    });

    CROW_ROUTE(app, "/api/v1/stream/lfsr/decrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string seed       = body["seed"].s();
        std::string ciphertext = body["ciphertext"].s();

        auto seed_bytes   = hex_to_bytes(seed);
        auto cipher_bytes = hex_to_bytes(ciphertext);

        if (seed_bytes.empty())   return err(400, "invalid seed");
        if (cipher_bytes.empty()) return err(400, "invalid ciphertext");

        StreamCipher sc(seed_bytes, StreamCipher::LFSR_JK);
        auto plain = sc.decrypt(cipher_bytes);

        return ok(crow::json::wvalue{
            {"plaintext", std::string(plain.begin(), plain.end())}
        });
    });

    // ── 4. DES ────────────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/des/encrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string dataHex = body["data"].s();
        std::string keyHex = body["key"].s();
        std::string modeStr = body["mode"].s();

        try {
            uint64_t key = std::stoull(keyHex, nullptr, 16);
            DES des(key);

            std::string dataStr;
            for (size_t i = 0; i < dataHex.length(); i += 2) {
                dataStr += static_cast<char>(std::stoi(dataHex.substr(i, 2), nullptr, 16));
            }
            std::vector<uint8_t> dataVec(dataStr.begin(), dataStr.end());

            DES::Mode mode = (modeStr == "CBC") ? DES::CBC : DES::ECB;
            uint64_t iv = 0;
            if (mode == DES::CBC && body.has("iv")) {
                iv = std::stoull(body["iv"].s(), nullptr, 16);
            }

            auto encrypted = des.encrypt(dataVec, mode, iv);
            std::string resultHex;
            for (uint8_t byte : encrypted) {
                char buf[3];
                snprintf(buf, sizeof(buf), "%02X", byte);
                resultHex += buf;
            }

            return ok(crow::json::wvalue{{"result", resultHex}});
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/des/decrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string dataHex = body["data"].s();
        std::string keyHex = body["key"].s();
        std::string modeStr = body["mode"].s();

        try {
            uint64_t key = std::stoull(keyHex, nullptr, 16);
            DES des(key);

            std::string dataStr;
            for (size_t i = 0; i < dataHex.length(); i += 2) {
                dataStr += static_cast<char>(std::stoi(dataHex.substr(i, 2), nullptr, 16));
            }
            std::vector<uint8_t> dataVec(dataStr.begin(), dataStr.end());

            DES::Mode mode = (modeStr == "CBC") ? DES::CBC : DES::ECB;
            uint64_t iv = 0;
            if (mode == DES::CBC && body.has("iv")) {
                iv = std::stoull(body["iv"].s(), nullptr, 16);
            }

            auto decrypted = des.decrypt(dataVec, mode, iv);
            std::string resultHex;
            for (uint8_t byte : decrypted) {
                char buf[3];
                snprintf(buf, sizeof(buf), "%02X", byte);
                resultHex += buf;
            }

            return ok(crow::json::wvalue{{"result", resultHex}});
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    // ── 5. RSA ────────────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/rsa/keygen")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        int p = body["p"].i();
        int q = body["q"].i();
        int e = body["e"].i();

        try {
            auto kp = RSA::generate((uint16_t)p, (uint16_t)q, (uint16_t)e);
            int64_t phi = (int64_t)((p - 1) * (q - 1));
            return ok(crow::json::wvalue{
                {"n",   (int64_t)std::stoull(kp.n.toDec())},
                {"e",   (int64_t)std::stoull(kp.e.toDec())},
                {"d",   (int64_t)std::stoull(kp.d.toDec())},
                {"phi", phi}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/rsa/encrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            std::string message = body["message"].s();
            int64_t n_val = body["key"]["n"].i();
            int64_t e_val = body["key"]["e"].i();

            RSA::KeyPair kp;
            kp.n = BigInt128(0ULL, (uint64_t)n_val);
            kp.e = BigInt128(0ULL, (uint64_t)e_val);
            kp.d = BigInt128(0ULL, 0ULL);

            RSA rsa(kp);
            auto blocks = rsa.encrypt(message);

            crow::json::wvalue::list block_list;
            block_list.reserve(blocks.size());
            for (const auto& b : blocks)
                block_list.emplace_back((int64_t)std::stoull(b.toDec()));

            return ok(crow::json::wvalue{{"blocks", std::move(block_list)}});
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/rsa/decrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            int64_t n_val = body["key"]["n"].i();
            int64_t d_val = body["key"]["d"].i();

            RSA::KeyPair kp;
            kp.n = BigInt128(0ULL, (uint64_t)n_val);
            kp.e = BigInt128(0ULL, 0ULL);
            kp.d = BigInt128(0ULL, (uint64_t)d_val);

            auto blocks_json = body["blocks"];
            std::vector<BigInt128> blocks;
            blocks.reserve(blocks_json.size());
            for (size_t i = 0; i < blocks_json.size(); i++)
                blocks.emplace_back(BigInt128(0ULL, (uint64_t)blocks_json[i].i()));

            RSA rsa(kp);
            return ok(crow::json::wvalue{{"message", rsa.decrypt(blocks)}});
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    // ── 6. D-H 协议 ───────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/dh/init")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            uint64_t p = std::stoull(std::string(body["p"].s()));
            uint64_t g = std::stoull(std::string(body["g"].s()));
            uint64_t clientPub = body.has("pubKey") ? std::stoull(std::string(body["pubKey"].s())) : 0;
            std::string nonce = body.has("nonce") ? std::string(body["nonce"].s()) : make_session_id("nonce_");

            if (p < 5 || g < 2 || g >= p) return err(400, "invalid DH parameters");
            if (clientPub <= 1 || clientPub >= p) return err(400, "invalid client public key");

            std::random_device rd;
            std::mt19937_64 gen(rd());
            std::uniform_int_distribution<uint64_t> dist(2, p - 2);

            DHServerSession session;
            session.p = p;
            session.g = g;
            session.clientPub = clientPub;
            session.serverPriv = dist(gen);
            session.serverPub = mod_pow_u64(g, session.serverPriv, p);
            session.nonce = nonce;
            session.message = "DH|p=" + std::to_string(p) +
                              "|g=" + std::to_string(g) +
                              "|client=" + std::to_string(clientPub) +
                              "|server=" + std::to_string(session.serverPub) +
                              "|nonce=" + nonce;

            const std::string digest = Hash::compute(session.message, Hash::SHA1);
            RSA signer(demo_signing_key());
            session.signature = signer.sign(digest_to_bigint(digest)).toHex();
            std::string sessionId = make_session_id("dh_");

            {
                std::lock_guard<std::mutex> lock(g_dh_mutex);
                g_dh_sessions[sessionId] = session;
            }

            auto kp = demo_signing_key();
            return ok(crow::json::wvalue{
                {"sessionId", sessionId},
                {"pubKey", std::to_string(session.serverPub)},
                {"nonce", nonce},
                {"message", session.message},
                {"hash", digest},
                {"signature", session.signature},
                {"rsaPublic", crow::json::wvalue{{"n", kp.n.toDec()}, {"e", kp.e.toDec()}}}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/dh/exchange")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            std::string sessionId = body.has("sessionId") ? std::string(body["sessionId"].s()) : "";
            DHServerSession session;
            {
                std::lock_guard<std::mutex> lock(g_dh_mutex);
                auto it = g_dh_sessions.find(sessionId);
                if (it == g_dh_sessions.end()) return err(404, "unknown DH session");
                session = it->second;
            }
            uint64_t shared = mod_pow_u64(session.clientPub, session.serverPriv, session.p);
            std::string fp = Hash::compute(std::to_string(shared), Hash::SHA1).substr(0, 16);
            return ok(crow::json::wvalue{{"sharedKey", std::to_string(shared)}, {"fingerprint", fp}});
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/dh/verify")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            std::string message = body.has("message") ? std::string(body["message"].s()) : "";
            std::string signature = body.has("signature") ? std::string(body["signature"].s()) : "";
            std::string digest = Hash::compute(message, Hash::SHA1);

            RSA::KeyPair kp = demo_signing_key();
            kp.d = BigInt128(0ULL, 0ULL);
            RSA verifier(kp);
            bool valid = verifier.verify(digest_to_bigint(digest), BigInt128::fromHex(signature));
            return ok(crow::json::wvalue{{"ok", valid}, {"hash", digest}});
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    // ── 7. 哈希 ───────────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/hash/compute")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            std::string message = body["message"].s();
            std::string algo    = body["algo"].s();  // "SHA1" or "MD5"
            return ok(crow::json::wvalue{{"hash", Hash::compute(message, parse_hash_algo(algo))}});
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    // ── 8. 数字签名 ───────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/sign/rsa")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            std::string message = body["message"].s();
            std::string algo    = body["algo"].s();
            Hash::Algo parsedAlgo = parse_hash_algo(algo);
            DigitalSignature ds(demo_signing_key());
            return ok(crow::json::wvalue{
                {"signature", ds.sign(message, parsedAlgo)},
                {"hash", Hash::compute(message, parsedAlgo)}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/sign/verify")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            std::string message   = body["message"].s();
            std::string signature = body["signature"].s();
            std::string algo      = body["algo"].s();
            Hash::Algo parsedAlgo = parse_hash_algo(algo);
            DigitalSignature ds(demo_signing_key());
            return ok(crow::json::wvalue{
                {"valid", ds.verify(message, signature, parsedAlgo)},
                {"hash", Hash::compute(message, parsedAlgo)}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    // ── 9. 大文件加密传输 ─────────────────────────────────
    CROW_ROUTE(app, "/api/v1/file/encrypt-upload")
    .methods("POST"_method)
    ([](const crow::request& req) {
        crow::multipart::message mp(req);

        std::string cipher     = "AES-256-CTR";
        std::string file_data;

        for (auto& part : mp.part_map) {
            if (part.first == "cipher")
                cipher = part.second.body;
            else if (part.first == "file")
                file_data = part.second.body;
        }

        if (file_data.empty()) return err(400, "missing file part");

        try {
            SecureFileTransfer sft;
            SecureFileTransfer::Config cfg;
            cfg.cipher = cipher;
            auto result = sft.receiveAndEncrypt(file_data, cipher, cfg);
            return ok(crow::json::wvalue{
                {"id", result.id},
                {"chunks", result.chunks},
                {"hash", result.hash},
                {"signature", result.signature}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/file/init")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        try {
            std::string name = body.has("name") ? std::string(body["name"].s()) : "upload.bin";
            uint64_t size = body.has("size") ? static_cast<uint64_t>(body["size"].i()) : 0ULL;
            std::string cipher = body.has("cipher") ? std::string(body["cipher"].s()) : "SHA1-CTR";
            size_t chunkSize = body.has("chunkSize") ? static_cast<size_t>(body["chunkSize"].i()) : 4 * 1024 * 1024;

            SecureFileTransfer::Config cfg;
            cfg.cipher = cipher;
            cfg.chunkSize = chunkSize;
            SecureFileTransfer sft;
            auto started = sft.startUpload(name, size, cipher, cfg);
            return ok(crow::json::wvalue{
                {"id", started.id},
                {"chunkSize", started.chunkSize},
                {"totalChunks", started.totalChunks},
                {"serverPubKey", started.serverPubKey},
                {"clientPubKey", started.clientPubKey},
                {"keyFingerprint", started.keyFingerprint},
                {"signature", started.signature}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/file/chunk/<string>/<int>")
    .methods("POST"_method)
    ([](const crow::request& req, const std::string& id, int index) {
        try {
            SecureFileTransfer sft;
            auto result = sft.receiveChunk(id, index, req.body);
            return ok(crow::json::wvalue{
                {"id", result.id},
                {"receivedChunks", result.receivedChunks},
                {"progress", result.progress},
                {"chunkHash", result.chunkHash},
                {"encryptedHash", result.encryptedHash}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/file/complete/<string>")
    .methods("POST"_method)
    ([](const std::string& id) {
        try {
            SecureFileTransfer sft;
            auto result = sft.finishUpload(id);
            return ok(crow::json::wvalue{
                {"id", result.id},
                {"chunks", result.chunks},
                {"hash", result.hash},
                {"signature", result.signature}
            });
        } catch (const std::exception& ex) {
            return err(400, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/file/status/<string>")
    .methods("GET"_method)
    ([](const std::string& id) {
        try {
            SecureFileTransfer sft;
            auto status = sft.getStatus(id);
            return ok(crow::json::wvalue{
                {"id", status.id},
                {"progress", status.progress},
                {"status", status.status}
            });
        } catch (const std::exception& ex) {
            return err(404, ex.what());
        }
    });

    CROW_ROUTE(app, "/api/v1/file/download/<string>")
    .methods("GET"_method)
    ([](const std::string& id) {
        try {
            SecureFileTransfer sft;
            std::string data = sft.getFile(id);
            crow::response res(200, data);
            res.set_header("Content-Type", "application/octet-stream");
            res.set_header("Content-Disposition", "attachment; filename=\"" + id + ".enc\"");
            set_cors(res);
            return res;
        } catch (const std::exception& ex) {
            return err(404, ex.what());
        }
    });

    // ── 前端静态文件托管 ──────────────────────────────────
    CROW_ROUTE(app, "/")
    ([]() {
        auto body = read_file(FRONTEND_DIR + "index.html");
        crow::response res(body.empty() ? 404 : 200, body);
        res.set_header("Content-Type", "text/html; charset=utf-8");
        return res;
    });

    CROW_ROUTE(app, "/<path>")
    .methods("GET"_method)
    ([](const std::string& path) {
        auto body = read_file(FRONTEND_DIR + path);
        crow::response res(body.empty() ? 404 : 200, body);
        res.set_header("Content-Type", mime_type(path));
        return res;
    });

    // ── 启动 ─────────────────────────────────────────────
    CROW_LOG_INFO << "Frontend: http://localhost:8888/";
    app.port(8888).multithreaded().run();
    return 0;
}
