/**
 * 局域网端到端加密核心算法 (JS 实现)
 * 必须与 C++ 端 SecureFileTransfer.cpp 的实现逻辑完全一致
 */

const DH_P = 65537n;
const DH_G = 3n;

/**
 * 模幂运算 (base^exp % mod)
 */
function modPow(base, exp, mod) {
    let res = 1n;
    base = base % mod;
    while (exp > 0n) {
        if (exp % 2n === 1n) res = (res * base) % mod;
        base = (base * base) % mod;
        exp = exp / 2n;
    }
    return res;
}

/**
 * 生成 DH 密钥对
 */
function generateDHKeyPair() {
    const priv = BigInt(Math.floor(Math.random() * Number(DH_P - 4n)) + 2);
    const pub = modPow(DH_G, priv, DH_P);
    return { priv, pub };
}

/**
 * 计算共享密钥
 */
function computeSharedKey(otherPub, myPriv) {
    return modPow(BigInt(otherPub), myPriv, DH_P);
}

/**
 * 计算 SHA-1 哈希 (16进制字符串)
 */
async function sha1Hex(text) {
    const encoder = new TextEncoder();
    const data = encoder.encode(text);
    const hashBuffer = await crypto.subtle.digest('SHA-1', data);
    const hashArray = Array.from(new Uint8Array(hashBuffer));
    return hashArray.map(b => b.toString(16).padStart(2, '0')).join('');
}

/**
 * 将 16 进制字符串转换为字节数组
 */
function hexToBytes(hex) {
    const bytes = new Uint8Array(hex.length / 2);
    for (let i = 0; i < hex.length; i += 2) {
        bytes[i / 2] = parseInt(hex.substr(i, 2), 16);
    }
    return bytes;
}

/**
 * XOR 流密码 (与 C++ 端 xorCrypt 一致)
 */
async function xorCrypt(data, key, id, chunkIndex) {
    const out = new Uint8Array(data);
    let pos = 0;
    let counter = 0;
    
    // data 是 ArrayBuffer 或 Uint8Array
    const dataView = new Uint8Array(data);

    while (pos < out.length) {
        const seed = `${key}|${id}|${chunkIndex}|${counter++}`;
        const streamHex = await sha1Hex(seed);
        const stream = hexToBytes(streamHex);
        
        for (let i = 0; i < stream.length && pos < out.length; i++, pos++) {
            out[pos] = dataView[pos] ^ stream[i];
        }
    }
    return out;
}
