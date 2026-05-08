// 抑制 OpenSSL 3.0 弃用警告（旧版 RSA API 仍可用）
#define OPENSSL_SUPPRESS_DEPRECATED

#include "JWT.h"
#include <openssl/pem.h>
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/sha.h>
#include <openssl/buffer.h>
#include <openssl/bio.h>
#include <sstream>
#include <cstring>
#include <ctime>

std::string JWT::Base64UrlEncode(const std::string& data) {
    BIO* bio = BIO_new(BIO_s_mem());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(bio, data.data(), data.length());
    BIO_flush(bio);
    
    BUF_MEM* bufferPtr;
    BIO_get_mem_ptr(bio, &bufferPtr);
    
    std::string result(bufferPtr->data, bufferPtr->length);
    BIO_free_all(bio);
    
    // 替换 + -> -, / -> _, 去掉 = 填充
    for (size_t i = 0; i < result.length(); ++i) {
        if (result[i] == '+') result[i] = '-';
        else if (result[i] == '/') result[i] = '_';
    }
    
    // 去掉末尾的 =
    size_t pos = result.find('=');
    if (pos != std::string::npos) {
        result = result.substr(0, pos);
    }
    
    return result;
}

std::string JWT::Base64UrlDecode(const std::string& data) {
    std::string padded = data;
    
    // 还原填充
    while (padded.length() % 4 != 0) {
        padded += '=';
    }
    
    // 替换 - -> +, _ -> /
    for (size_t i = 0; i < padded.length(); ++i) {
        if (padded[i] == '-') padded[i] = '+';
        else if (padded[i] == '_') padded[i] = '/';
    }
    
    BIO* bio = BIO_new_mem_buf(padded.data(), padded.length());
    BIO* b64 = BIO_new(BIO_f_base64());
    bio = BIO_push(b64, bio);
    
    BIO_set_flags(bio, BIO_FLAGS_BASE64_NO_NL);
    
    std::vector<char> buffer(padded.length());
    int decodedLen = BIO_read(bio, buffer.data(), padded.length());
    BIO_free_all(bio);
    
    return std::string(buffer.data(), decodedLen);
}

std::string JWT::GenerateHeader() {
    // JWT 头部: {"alg":"RS256","typ":"JWT"}
    return "{\"alg\":\"RS256\",\"typ\":\"JWT\"}";
}

std::string JWT::Generate(const std::string& userId,
                          const std::string& privateKey,
                          int expireSeconds) {
    return GenerateWithPayload(userId, "normal", privateKey, expireSeconds);
}

std::string JWT::GenerateWithPayload(const std::string& userId,
                                     const std::string& role,
                                     const std::string& privateKey,
                                     int expireSeconds) {
    // 1. 生成头部
    std::string header = GenerateHeader();
    std::string headerB64 = Base64UrlEncode(header);
    
    // 2. 生成载荷（包含角色）
    std::time_t now = std::time(nullptr);
    std::time_t exp = now + expireSeconds;
    
    std::stringstream payload;
    payload << "{\"sub\":\"" << userId << "\","
            << "\"role\":\"" << role << "\","
            << "\"iat\":" << now << ","
            << "\"exp\":" << exp << "}";
    
    std::string payloadStr = payload.str();
    std::string payloadB64 = Base64UrlEncode(payloadStr);
    
    // 3. 生成签名数据
    std::string signingInput = headerB64 + "." + payloadB64;
    
    // 4. 使用 RSA 私钥签名
    BIO* bio = BIO_new_mem_buf(privateKey.data(), privateKey.length());
    if (!bio) return "";
    
    RSA* rsa = PEM_read_bio_RSAPrivateKey(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!rsa) return "";
    
    // 使用 SHA-256 哈希
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(signingInput.data()), 
           signingInput.length(), hash);
    
    // RSA 签名
    unsigned char signature[256];  // RSA 2048 = 256 bytes
    unsigned int sigLen = 0;
    
    int result = RSA_sign(NID_sha256, hash, SHA256_DIGEST_LENGTH,
                          signature, &sigLen, rsa);
    RSA_free(rsa);
    
    if (result != 1) return "";
    
    std::string sigStr(reinterpret_cast<char*>(signature), sigLen);
    std::string signatureB64 = Base64UrlEncode(sigStr);
    
    // 5. 组合 JWT
    return signingInput + "." + signatureB64;
}

bool JWT::Verify(const std::string& token, const std::string& publicKey) {
    // 分割 token
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    
    if (firstDot == std::string::npos || secondDot == std::string::npos) {
        return false;
    }
    
    std::string headerB64 = token.substr(0, firstDot);
    std::string payloadB64 = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string signatureB64 = token.substr(secondDot + 1);
    
    std::string signingInput = headerB64 + "." + payloadB64;
    std::string signature = Base64UrlDecode(signatureB64);
    
    // 加载公钥
    BIO* bio = BIO_new_mem_buf(publicKey.data(), publicKey.length());
    if (!bio) return false;
    
    RSA* rsa = PEM_read_bio_RSA_PUBKEY(bio, nullptr, nullptr, nullptr);
    BIO_free(bio);
    
    if (!rsa) return false;
    
    // 计算哈希并验证签名
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(reinterpret_cast<const unsigned char*>(signingInput.data()), 
           signingInput.length(), hash);
    
    int result = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH,
                            reinterpret_cast<const unsigned char*>(signature.data()),
                            signature.length(), rsa);
    RSA_free(rsa);
    
    if (result != 1) return false;
    
    // 检查过期时间
    std::string payloadJson = Base64UrlDecode(payloadB64);
    size_t expPos = payloadJson.find("\"exp\":");
    if (expPos != std::string::npos) {
        size_t start = expPos + 6;
        size_t end = payloadJson.find_first_of(",}", start);
        std::string expStr = payloadJson.substr(start, end - start);
        std::time_t exp = std::stoll(expStr);
        
        if (std::time(nullptr) > exp) {
            return false;  // 已过期
        }
    }
    
    return true;
}

std::map<std::string, std::string> JWT::ParsePayload(const std::string& token) {
    std::map<std::string, std::string> result;
    
    size_t firstDot = token.find('.');
    size_t secondDot = token.find('.', firstDot + 1);
    
    if (firstDot == std::string::npos || secondDot == std::string::npos) {
        return result;
    }
    
    std::string payloadB64 = token.substr(firstDot + 1, secondDot - firstDot - 1);
    std::string payloadJson = Base64UrlDecode(payloadB64);
    
    // 简单解析 JSON (简化版)
    size_t pos = 0;
    while ((pos = payloadJson.find('\"', pos)) != std::string::npos) {
        size_t keyEnd = payloadJson.find('\"', pos + 1);
        if (keyEnd == std::string::npos) break;
        
        std::string key = payloadJson.substr(pos + 1, keyEnd - pos - 1);
        
        size_t colonPos = payloadJson.find(':', keyEnd);
        if (colonPos == std::string::npos) break;
        
        size_t valueStart = payloadJson.find_first_not_of(" \t", colonPos + 1);
        if (valueStart == std::string::npos) break;
        
        std::string value;
        if (payloadJson[valueStart] == '\"') {
            size_t valueEnd = payloadJson.find('\"', valueStart + 1);
            value = payloadJson.substr(valueStart + 1, valueEnd - valueStart - 1);
            pos = valueEnd + 1;
        } else {
            size_t valueEnd = payloadJson.find_first_of(",}", valueStart);
            value = payloadJson.substr(valueStart, valueEnd - valueStart);
            pos = valueEnd;
        }
        
        result[key] = value;
    }
    
    return result;
}
