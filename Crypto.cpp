#include "Crypto.h"
#include <openssl/evp.h>
#include <openssl/rand.h>
#include <openssl/buffer.h>
#include <sstream>
#include <iomanip>
#include <cstring>

// 使用 OpenSSL 实现 bcrypt 风格的密码哈希
// 格式: $2a$cost$salt+hash

static std::string Base64Encode(const unsigned char* data, size_t length) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data, length);
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    return result;
}

static std::string Base64Decode(const std::string& encoded) {
    BIO* bio = BIO_new_mem_buf(encoded.data(), encoded.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    
    std::vector<char> buffer(encoded.length());
    int decodedLen = BIO_read(bio, buffer.data(), encoded.length());
    BIO_free_all(bio);
    
    return std::string(buffer.data(), decodedLen);
}

std::string Crypto::HashPassword(const std::string& password) {
    // 生成随机盐值 (16字节)
    unsigned char salt[16];
    if (RAND_bytes(salt, sizeof(salt)) != 1) {
        return "";
    }
    
    // 使用 PBKDF2 进行密钥派生
    unsigned char hash[32];
    if (!PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           salt, sizeof(salt),
                           1 << BCRYPT_WORK_FACTOR,  // 2^work_factor 次迭代
                           EVP_sha256(),
                           sizeof(hash), hash)) {
        return "";
    }
    
    // 编码格式: $pbkdf2$cost$salt$hash
    std::stringstream result;
    result << "$pbkdf2$" << BCRYPT_WORK_FACTOR << "$";
    result << Base64Encode(salt, sizeof(salt)) << "$";
    result << Base64Encode(hash, sizeof(hash));
    
    return result.str();
}

bool Crypto::VerifyPassword(const std::string& password, const std::string& hash) {
    // 解析哈希字符串
    if (hash.substr(0, 8) != "$pbkdf2$") {
        return false;
    }
    
    size_t costPos = hash.find('$', 8);
    if (costPos == std::string::npos) return false;
    
    size_t saltPos = hash.find('$', costPos + 1);
    if (saltPos == std::string::npos) return false;
    
    int cost = std::stoi(hash.substr(8, costPos - 8));
    std::string saltB64 = hash.substr(costPos + 1, saltPos - costPos - 1);
    std::string hashB64 = hash.substr(saltPos + 1);
    
    std::string salt = Base64Decode(saltB64);
    
    // 重新计算哈希
    unsigned char newHash[32];
    if (!PKCS5_PBKDF2_HMAC(password.c_str(), password.length(),
                           reinterpret_cast<const unsigned char*>(salt.data()), salt.length(),
                           1 << cost,
                           EVP_sha256(),
                           sizeof(newHash), newHash)) {
        return false;
    }
    
    std::string newHashB64 = Base64Encode(newHash, sizeof(newHash));
    
    // 使用常量时间比较防止时序攻击
    if (newHashB64.length() != hashB64.length()) {
        return false;
    }
    
    volatile int result = 0;
    for (size_t i = 0; i < hashB64.length(); ++i) {
        result |= (newHashB64[i] ^ hashB64[i]);
    }
    
    return result == 0;
}
