#define CROW_MAIN
#include "crow_all.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <unordered_map>

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
    CROW_ROUTE(app, "/api/v1/stream/rc4")
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

    CROW_ROUTE(app, "/api/v1/stream/lfsr")
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

    // ── 4. DES ────────────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/des/encrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string data = body["data"].s();   // HEX string
        std::string key  = body["key"].s();    // 16 HEX chars
        std::string mode = body["mode"].s();   // "ECB" or "CBC"

        // TODO: uint64_t k = std::stoull(key, nullptr, 16);
        //       DES des(k);
        //       auto result = des.encrypt(data_bytes, mode == "CBC" ? DES::CBC : DES::ECB);
        //       return ok({{"result", to_hex(result)}});
        return ok(crow::json::wvalue{{"result", "NOT_IMPLEMENTED"}});
    });

    CROW_ROUTE(app, "/api/v1/des/decrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string data = body["data"].s();
        std::string key  = body["key"].s();
        std::string mode = body["mode"].s();

        // TODO: DES des(k); auto result = des.decrypt(...);
        return ok(crow::json::wvalue{{"result", "NOT_IMPLEMENTED"}});
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

        std::string p      = body["p"].s();
        std::string g      = body["g"].s();
        std::string pubKey = body["pubKey"].s();
        std::string nonce  = body["nonce"].s();

        // TODO: DHProtocol dh(BigInt128(p), BigInt128(g));
        //       auto serverPub = dh.generatePublicKey();
        //       return ok({{"pubKey", serverPub.toDec()}, {"signature", "..."}});
        return ok(crow::json::wvalue{{"pubKey", "NOT_IMPLEMENTED"}, {"signature", "NOT_IMPLEMENTED"}});
    });

    CROW_ROUTE(app, "/api/v1/dh/exchange")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        // TODO: auto shared = dh.computeSharedKey(peerPublic);
        return ok(crow::json::wvalue{{"sharedKey", "NOT_IMPLEMENTED"}});
    });

    CROW_ROUTE(app, "/api/v1/dh/verify")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        // TODO: bool ok = dh.verifyPeer(signedMsg, peerPubKey);
        return ok(crow::json::wvalue{{"ok", false}});
    });

    // ── 7. 哈希 ───────────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/hash/compute")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string message = body["message"].s();
        std::string algo    = body["algo"].s();  // "SHA1" or "MD5"

        // TODO: auto h = Hash::compute(message, algo == "MD5" ? Hash::MD5 : Hash::SHA1);
        return ok(crow::json::wvalue{{"hash", "NOT_IMPLEMENTED"}});
    });

    // ── 8. 数字签名 ───────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/sign/rsa")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string message = body["message"].s();
        std::string algo    = body["algo"].s();

        // TODO: DigitalSignature ds(key_pair);
        //       auto sig = ds.sign(message, algo == "MD5" ? Hash::MD5 : Hash::SHA1);
        //       return ok({{"signature", sig}, {"hash", hash}});
        return ok(crow::json::wvalue{{"signature", "NOT_IMPLEMENTED"}, {"hash", "NOT_IMPLEMENTED"}});
    });

    CROW_ROUTE(app, "/api/v1/sign/verify")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string message   = body["message"].s();
        std::string signature = body["signature"].s();
        std::string algo      = body["algo"].s();

        // TODO: DigitalSignature ds(key_pair);
        //       bool valid = ds.verify(message, signature, algo == "MD5" ? Hash::MD5 : Hash::SHA1);
        //       return ok({{"valid", valid}, {"hash", hash}});
        return ok(crow::json::wvalue{{"valid", false}, {"hash", "NOT_IMPLEMENTED"}});
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

        // TODO: SecureFileTransfer sft;
        //       auto id = sft.receiveAndEncrypt(file_data, cipher);
        //       return ok({{"id", id}, {"chunks", ...}, {"hash", ...}});
        std::string stub_id = "tx_" + std::to_string(std::time(nullptr));
        return ok(crow::json::wvalue{
            {"id",     stub_id},
            {"chunks", 1},
            {"hash",   "NOT_IMPLEMENTED"}
        });
    });

    CROW_ROUTE(app, "/api/v1/file/status/<string>")
    .methods("GET"_method)
    ([](const std::string& id) {
        // TODO: auto status = sft.getStatus(id); return ok({{"id",id},{"progress",status.progress},...});
        return ok(crow::json::wvalue{
            {"id",       id},
            {"progress", 100},
            {"status",   "done"}
        });
    });

    CROW_ROUTE(app, "/api/v1/file/download/<string>")
    .methods("GET"_method)
    ([](const std::string& id) {
        // TODO: auto data = sft.getFile(id); return binary response;
        crow::response res(501, "NOT_IMPLEMENTED");
        set_cors(res);
        return res;
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
