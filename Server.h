#ifndef SERVER_H
#define SERVER_H

#include "AuthService.h"
#include <string>
#include <map>
#include <functional>

// HTTP 请求结构
struct HttpRequest {
    std::string method;
    std::string path;
    std::map<std::string, std::string> headers;
    std::string body;
    std::map<std::string, std::string> queryParams;
};

// HTTP 响应结构
struct HttpResponse {
    int statusCode;
    std::map<std::string, std::string> headers;
    std::string body;
    
    HttpResponse() : statusCode(200) {
        headers["Content-Type"] = "application/json";
        headers["Connection"] = "close";
    }
};

// 单线程 epoll HTTP 服务器
class Server {
public:
    // 构造函数
    // port: 监听端口
    // authService: 认证服务实例
    Server(int port, AuthService* authService);
    
    ~Server();
    
    // 启动服务器 (阻塞)
    void Start();
    
    // 停止服务器
    void Stop();

private:
    // 创建监听 socket
    bool CreateListenSocket();
    
    // 设置非阻塞
    bool SetNonBlocking(int fd);
    
    // 处理新连接
    void HandleNewConnection();
    
    // 处理客户端数据
    void HandleClientData(int clientFd);
    
    // 关闭客户端连接
    void CloseClient(int clientFd);
    
    // 解析 HTTP 请求
    HttpRequest ParseHttpRequest(const std::string& rawRequest);
    
    // 构建 HTTP 响应
    std::string BuildHttpResponse(const HttpResponse& response);
    
    // 处理请求路由
    HttpResponse RouteRequest(const HttpRequest& request);
    
    // 处理登录请求
    HttpResponse HandleLogin(const HttpRequest& request);
    
    // 处理注册请求
    HttpResponse HandleRegister(const HttpRequest& request);
    
    // 处理管理员注册请求（可指定角色）
    HttpResponse HandleAdminRegister(const HttpRequest& request);
    
    // 处理健康检查
    HttpResponse HandleHealth(const HttpRequest& request);
    
    // 处理获取用户信息
    HttpResponse HandleGetUserInfo(const HttpRequest& request);
    
    // URL 解码
    std::string UrlDecode(const std::string& encoded);
    
    // 解析 JSON (简化版)
    std::map<std::string, std::string> ParseJson(const std::string& json);
    
    // 构建 JSON
    std::string BuildJson(const std::map<std::string, std::string>& data);

private:
    int port_;
    int listenFd_;
    int epollFd_;
    AuthService* authService_;
    bool running_;
    
    // 客户端缓冲区
    std::map<int, std::string> clientBuffers_;
    
    // 客户端信息 (fd -> IP:port)
    std::map<int, std::string> clientInfos_;
    
    static const int MAX_EVENTS = 1024;
    static const int BUFFER_SIZE = 4096;
};

#endif // SERVER_H
