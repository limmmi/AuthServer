#include "AuthService.h"
#include "Server.h"
#include "Logger.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

// 全局服务器指针用于信号处理
static Server* g_server = nullptr;

void SignalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        LOG_INFO("Received signal {}, shutting down...", sig);
        if (g_server) {
            g_server->Stop();
        }
    }
}

void PrintUsage(const char* program) {
    std::cout << "Usage: " << program << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  -p <port>         Listen port (default: 8081)" << std::endl;
    std::cout << "  -k <keyfile>      RSA private key file (default: ./private.key)" << std::endl;
    std::cout << "  -d <dbpath>       Database path (default: ./auth_db)" << std::endl;
    std::cout << "  -e <seconds>      Token expire time in seconds (default: 86400)" << std::endl;
    std::cout << "  -h                Show this help" << std::endl;
    std::cout << std::endl;
    std::cout << "Example:" << std::endl;
    std::cout << "  " << program << " -p 8081 -k ./private.key -d ./auth_db" << std::endl;
}

int main(int argc, char* argv[]) {
    // 默认配置
    int port = 8081;
    std::string privateKeyPath = "./private.key";
    std::string dbPath = "./auth_db";
    int tokenExpireSeconds = 86400;  // 24小时
    
    // 解析命令行参数
    int opt;
    while ((opt = getopt(argc, argv, "p:k:d:e:h")) != -1) {
        switch (opt) {
            case 'p':
                port = std::atoi(optarg);
                if (port <= 0 || port > 65535) {
                    std::cerr << "Invalid port number: " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'k':
                privateKeyPath = optarg;
                break;
            case 'd':
                dbPath = optarg;
                break;
            case 'e':
                tokenExpireSeconds = std::atoi(optarg);
                if (tokenExpireSeconds <= 0) {
                    std::cerr << "Invalid expire time: " << optarg << std::endl;
                    return 1;
                }
                break;
            case 'h':
                PrintUsage(argv[0]);
                return 0;
            default:
                PrintUsage(argv[0]);
                return 1;
        }
    }
    
    // 初始化日志系统
    if (!Logger::Initialize("./logs/auth_server.log", "info")) {
        std::cerr << "Failed to initialize logger" << std::endl;
        return 1;
    }
    
    LOG_INFO("========================================");
    LOG_INFO("       Auth Server Starting...          ");
    LOG_INFO("========================================");
    LOG_INFO("Port: {}", port);
    LOG_INFO("Private Key: {}", privateKeyPath);
    LOG_INFO("Database: {}", dbPath);
    LOG_INFO("Token Expire: {} seconds", tokenExpireSeconds);
    LOG_INFO("========================================");
    
    // 初始化认证服务
    AuthService authService(privateKeyPath, tokenExpireSeconds, dbPath);
    if (!authService.Initialize()) {
        LOG_ERROR("Failed to initialize auth service");
        Logger::Shutdown();
        return 1;
    }
    
    // 创建并启动服务器
    Server server(port, &authService);
    g_server = &server;
    
    // 注册信号处理
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGPIPE, SIG_IGN);  // 忽略管道破裂信号
    
    LOG_INFO("Server is running. Press Ctrl+C to stop.");
    
    // 启动服务器 (阻塞)
    server.Start();
    
    LOG_INFO("Server stopped.");
    
    // 关闭日志系统
    Logger::Shutdown();
    
    return 0;
}
