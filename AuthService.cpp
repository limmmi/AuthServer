#include "AuthService.h"
#include "Crypto.h"
#include "JWT.h"
#include "Logger.h"
#include <fstream>
#include <sstream>
#include <iostream>
#include <ctime>

// 第一个注册的用户自动成为管理员
static bool g_firstUserIsAdmin = true;

AuthService::AuthService(const std::string& privateKeyPath,
                         int tokenExpireSeconds,
                         const std::string& dbPath)
    : privateKeyPath_(privateKeyPath)
    , tokenExpireSeconds_(tokenExpireSeconds)
    , dbPath_(dbPath)
    , nextUserId_(1) {
}

AuthService::~AuthService() {
    db_.Close();
}

bool AuthService::Initialize() {
    // 加载私钥
    if (!LoadPrivateKey()) {
        std::cerr << "Failed to load private key from: " << privateKeyPath_ << std::endl;
        return false;
    }
    
    // 打开数据库
    if (!db_.Open(dbPath_)) {
        std::cerr << "Failed to open database at: " << dbPath_ << std::endl;
        return false;
    }
    
    // 读取当前最大用户ID
    std::string maxIdStr;
    if (db_.Get("__max_user_id__", maxIdStr)) {
        nextUserId_ = std::stoi(maxIdStr) + 1;
        g_firstUserIsAdmin = false;  // 已有用户，不再是第一个
    }
    
    LOG_INFO("AuthService initialized, next user ID: {}", nextUserId_);
    return true;
}

bool AuthService::LoadPrivateKey() {
    std::ifstream file(privateKeyPath_);
    if (!file.is_open()) {
        return false;
    }
    
    std::stringstream buffer;
    buffer << file.rdbuf();
    privateKey_ = buffer.str();
    
    return !privateKey_.empty();
}

std::string AuthService::GenerateUserId() {
    std::string userId = std::to_string(nextUserId_++);
    // 保存新的最大ID
    db_.Put("__max_user_id__", std::to_string(nextUserId_ - 1));
    return userId;
}

std::string AuthService::RoleToString(UserRole role) {
    return (role == UserRole::ADMIN) ? "admin" : "normal";
}

UserRole AuthService::StringToRole(const std::string& str) {
    return (str == "admin") ? UserRole::ADMIN : UserRole::NORMAL;
}

std::string AuthService::GenerateJWT(const std::string& userId, UserRole role) {
    return JWT::GenerateWithPayload(userId, RoleToString(role), privateKey_, tokenExpireSeconds_);
}

bool AuthService::ParseUserData(const std::string& data, UserInfo& info) {
    // 格式: userId:role:passwordHash
    size_t firstSep = data.find(':');
    if (firstSep == std::string::npos) return false;
    
    size_t secondSep = data.find(':', firstSep + 1);
    if (secondSep == std::string::npos) {
        // 兼容旧格式 (无角色): userId:passwordHash，默认为 normal
        info.userId = data.substr(0, firstSep);
        info.role = UserRole::NORMAL;
        info.passwordHash = data.substr(firstSep + 1);
        return true;
    }
    
    info.userId = data.substr(0, firstSep);
    info.role = StringToRole(data.substr(firstSep + 1, secondSep - firstSep - 1));
    info.passwordHash = data.substr(secondSep + 1);
    return true;
}

std::string AuthService::SerializeUserData(const UserInfo& info) {
    return info.userId + ":" + RoleToString(info.role) + ":" + info.passwordHash;
}

bool AuthService::GetUserInfo(const std::string& username, UserInfo& info) {
    std::string userData;
    if (!db_.Get(username, userData)) {
        return false;
    }
    return ParseUserData(userData, info);
}

bool AuthService::VerifyAdmin(const std::string& token) {
    // 解析 token 获取角色
    auto payload = JWT::ParsePayload(token);
    auto it = payload.find("role");
    if (it == payload.end()) {
        return false;
    }
    return it->second == "admin";
}

std::string AuthService::Login(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        LOG_WARN("Login failed: empty username or password");
        return "";
    }
    
    UserInfo info;
    if (!GetUserInfo(username, info)) {
        LOG_WARN("Login failed: user not found - {}", username);
        return "";  // 用户不存在
    }
    
    // 验证密码
    if (!Crypto::VerifyPassword(password, info.passwordHash)) {
        LOG_WARN("Login failed: wrong password - {}", username);
        return "";  // 密码错误
    }
    
    // 生成并返回 JWT（包含角色）
    std::string token = GenerateJWT(info.userId, info.role);
    LOG_INFO("Login success: {} (role: {})", username, RoleToString(info.role));
    return token;
}

bool AuthService::Register(const std::string& username, const std::string& password) {
    if (username.empty() || password.empty()) {
        LOG_WARN("Register failed: empty username or password");
        return false;
    }
    
    if (username.length() < 3 || username.length() > 32) {
        LOG_WARN("Register failed: username length invalid - {}", username);
        return false;
    }
    
    if (password.length() < 6 || password.length() > 128) {
        LOG_WARN("Register failed: password length invalid - {}", username);
        return false;
    }
    
    if (db_.Exists(username)) {
        LOG_WARN("Register failed: user already exists - {}", username);
        return false;
    }
    
    // 第一个用户自动成为管理员
    UserRole role = g_firstUserIsAdmin ? UserRole::ADMIN : UserRole::NORMAL;
    if (g_firstUserIsAdmin) {
        g_firstUserIsAdmin = false;
        LOG_INFO("First user registered as ADMIN: {}", username);
    }
    
    UserInfo info;
    info.userId = GenerateUserId();
    info.role = role;
    info.passwordHash = Crypto::HashPassword(password);
    
    if (info.passwordHash.empty()) {
        LOG_ERROR("Register failed: password hash failed - {}", username);
        return false;
    }
    
    if (!db_.Put(username, SerializeUserData(info))) {
        LOG_ERROR("Register failed: database error - {}", username);
        return false;
    }
    
    LOG_INFO("User registered: {} (role: {}, id: {})", username, RoleToString(role), info.userId);
    return true;
}

bool AuthService::RegisterWithRole(const std::string& username, 
                                   const std::string& password, 
                                   UserRole role,
                                   const std::string& adminToken) {
    // 验证调用者是否为管理员
    if (!VerifyAdmin(adminToken)) {
        LOG_WARN("Admin register failed: invalid admin token");
        return false;
    }
    
    if (username.empty() || password.empty()) {
        LOG_WARN("Admin register failed: empty username or password");
        return false;
    }
    
    if (username.length() < 3 || username.length() > 32) {
        LOG_WARN("Admin register failed: username length invalid - {}", username);
        return false;
    }
    
    if (password.length() < 6 || password.length() > 128) {
        LOG_WARN("Admin register failed: password length invalid - {}", username);
        return false;
    }
    
    if (db_.Exists(username)) {
        LOG_WARN("Admin register failed: user already exists - {}", username);
        return false;
    }
    
    UserInfo info;
    info.userId = GenerateUserId();
    info.role = role;
    info.passwordHash = Crypto::HashPassword(password);
    
    if (info.passwordHash.empty()) {
        LOG_ERROR("Admin register failed: password hash failed - {}", username);
        return false;
    }
    
    if (!db_.Put(username, SerializeUserData(info))) {
        LOG_ERROR("Admin register failed: database error - {}", username);
        return false;
    }
    
    LOG_INFO("Admin registered new user: {} (role: {}, id: {})", username, RoleToString(role), info.userId);
    return true;
}
