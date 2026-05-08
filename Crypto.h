#ifndef CRYPTO_H
#define CRYPTO_H

#include <string>
#include <vector>

// 密码加密类 - 使用 bcrypt 算法
class Crypto {
public:
    // 生成密码哈希
    static std::string HashPassword(const std::string& password);
    
    // 验证密码
    static bool VerifyPassword(const std::string& password, const std::string& hash);
    
private:
    // bcrypt 工作因子 (4-31, 越大越慢越安全)
    static const int BCRYPT_WORK_FACTOR = 12;
};

#endif // CRYPTO_H
