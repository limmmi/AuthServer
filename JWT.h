#ifndef JWT_H
#define JWT_H

#include <string>
#include <map>

// JWT 令牌生成类 - 使用 RSA256 签名
class JWT {
public:
    // 生成 JWT 令牌
    // userId: 用户ID
    // privateKey: RSA 私钥 (PEM 格式)
    // expireSeconds: 过期时间(秒)
    static std::string Generate(const std::string& userId,
                                const std::string& privateKey,
                                int expireSeconds);
    
    // 生成 JWT 令牌（带角色）
    static std::string GenerateWithPayload(const std::string& userId,
                                           const std::string& role,
                                           const std::string& privateKey,
                                           int expireSeconds);
    
    // 验证 JWT 令牌 (使用公钥)
    static bool Verify(const std::string& token, const std::string& publicKey);
    
    // 解析 JWT 载荷 (不验证签名)
    static std::map<std::string, std::string> ParsePayload(const std::string& token);

private:
    // Base64 URL 编码 (无填充)
    static std::string Base64UrlEncode(const std::string& data);
    static std::string Base64UrlDecode(const std::string& data);
    
    // 生成 JWT 头部
    static std::string GenerateHeader();
};

#endif // JWT_H
