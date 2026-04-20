#define CROW_MAIN
#include "crow_all.h"
#include <fstream>
#include <sstream>
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

        // TODO: AffineCipher cipher(a, b); return ok({{"ciphertext", cipher.encrypt(plaintext)}, ...});
        return ok(crow::json::wvalue{
            {"ciphertext", "NOT_IMPLEMENTED"},
            {"key", crow::json::wvalue{{"a", a}, {"b", b}}}
        });
    });

    CROW_ROUTE(app, "/api/v1/affine/decrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string ciphertext = body["ciphertext"].s();
        int a = body["a"].i();
        int b = body["b"].i();

        // TODO: AffineCipher cipher(a, b); return ok({{"plaintext", cipher.decrypt(ciphertext)}});
        return ok(crow::json::wvalue{{"plaintext", "NOT_IMPLEMENTED"}});
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

        // TODO: BigInt128 ba(a), bb(b); auto r = ...; return ok({{"result", r.toDec()}, {"hex", r.toHex()}});
        return ok(crow::json::wvalue{{"result", "NOT_IMPLEMENTED"}, {"hex", "NOT_IMPLEMENTED"}});
    });

    // ── 3. 流密码 ─────────────────────────────────────────
    CROW_ROUTE(app, "/api/v1/stream/rc4")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string seed      = body["seed"].s();
        std::string plaintext = body["plaintext"].s();

        // TODO: StreamCipher sc(seed_bytes, StreamCipher::RC4);
        //       auto cipher = sc.encrypt(plaintext_bytes);
        //       return ok({{"ciphertext", hex_str}, {"keystream", ks_str}});
        return ok(crow::json::wvalue{{"ciphertext", "NOT_IMPLEMENTED"}, {"keystream", "NOT_IMPLEMENTED"}});
    });

    CROW_ROUTE(app, "/api/v1/stream/lfsr")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        std::string seed      = body["seed"].s();
        std::string plaintext = body["plaintext"].s();

        // TODO: StreamCipher sc(seed_bytes, StreamCipher::LFSR_JK);
        return ok(crow::json::wvalue{{"ciphertext", "NOT_IMPLEMENTED"}, {"keystream", "NOT_IMPLEMENTED"}});
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

        // TODO: auto kp = RSA::generate(p, q, e);
        //       return ok({{"n", kp.n.toDec()}, {"e", kp.e.toDec()}, {"d", kp.d.toDec()}});
        return ok(crow::json::wvalue{{"n", 0}, {"e", 0}, {"d", 0}, {"phi", 0}});
    });

    CROW_ROUTE(app, "/api/v1/rsa/encrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        // TODO: RSA rsa(key_pair); auto blocks = rsa.encrypt(message); return ok({{"blocks", ...}});
        return ok(crow::json::wvalue{{"blocks", crow::json::wvalue::list{}}});
    });

    CROW_ROUTE(app, "/api/v1/rsa/decrypt")
    .methods("POST"_method)
    ([](const crow::request& req) {
        auto body = crow::json::load(req.body);
        if (!body) return err(400, "invalid JSON");

        // TODO: RSA rsa(key_pair); return ok({{"message", rsa.decrypt(blocks)}});
        return ok(crow::json::wvalue{{"message", "NOT_IMPLEMENTED"}});
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
    CROW_LOG_INFO << "Frontend: http://localhost:8080/";
    app.port(8080).multithreaded().run();
    return 0;
}
