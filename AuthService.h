#ifndef AUTH_SERVICE_H
#define AUTH_SERVICE_H

#include "Database.h"
#include <string>

// 用户角色枚举
enum class UserRole {
    NORMAL = 0,    // 普通用户
    ADMIN = 1      // 管理员
};

// 用户信息结构
struct UserInfo {
    std::string userId;
    UserRole role;
    std::string passwordHash;
};

// 认证服务核心类
class AuthService {
public:
    // 构造函数
    // privateKeyPath: RSA 私钥文件路径
    // tokenExpireSeconds: JWT 过期时间(秒)
    // dbPath: LevelDB 数据库路径
    AuthService(const std::string& privateKeyPath, 
                int tokenExpireSeconds,
                const std::string& dbPath);
    
    ~AuthService();
    
    // 初始化
    bool Initialize();
    
    // 用户登录
    // 返回: JWT 令牌, 失败返回空字符串
    std::string Login(const std::string& username, const std::string& password);
    
    // 用户注册 (普通用户)
    // 返回: true-成功, false-失败(用户已存在等)
    bool Register(const std::string& username, const std::string& password);
    
    // 管理员注册 (可指定角色)
    // 需要管理员权限才能调用
    bool RegisterWithRole(const std::string& username, 
                          const std::string& password, 
                          UserRole role,
                          const std::string& adminToken);
    
    // 获取用户信息
    bool GetUserInfo(const std::string& username, UserInfo& info);
    
    // 生成用户 ID (简单自增)
    std::string GenerateUserId();
    
    // 角色转字符串
    static std::string RoleToString(UserRole role);
    static UserRole StringToRole(const std::string& str);

private:
    // 从文件加载私钥
    bool LoadPrivateKey();
    
    // 生成 JWT (包含角色信息)
    std::string GenerateJWT(const std::string& userId, UserRole role);
    
    // 验证管理员权限
    bool VerifyAdmin(const std::string& token);
    
    // 解析用户数据
    bool ParseUserData(const std::string& data, UserInfo& info);
    
    // 序列化用户数据
    std::string SerializeUserData(const UserInfo& info);

private:
    std::string privateKeyPath_;
    std::string privateKey_;
    int tokenExpireSeconds_;
    std::string dbPath_;
    Database db_;
    int nextUserId_;
};

#endif // AUTH_SERVICE_H
