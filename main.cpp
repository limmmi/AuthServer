#include "AuthService.h"
#include "Server.h"
#include <iostream>
#include <csignal>
#include <cstdlib>
#include <unistd.h>

// 全局服务器指针用于信号处理
static Server* g_server = nullptr;

void SignalHandler(int sig) {
    if (sig == SIGINT || sig == SIGTERM) {
        std::cout << "\nReceived signal " << sig << ", shutting down..." << std::endl;
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
    
    std::cout << "========================================" << std::endl;
    std::cout << "       Auth Server Starting...          " << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "Private Key: " << privateKeyPath << std::endl;
    std::cout << "Database: " << dbPath << std::endl;
    std::cout << "Token Expire: " << tokenExpireSeconds << " seconds" << std::endl;
    std::cout << "========================================" << std::endl;
    
    // 初始化认证服务
    AuthService authService(privateKeyPath, tokenExpireSeconds, dbPath);
    if (!authService.Initialize()) {
        std::cerr << "Failed to initialize auth service" << std::endl;
        return 1;
    }
    
    // 创建并启动服务器
    Server server(port, &authService);
    g_server = &server;
    
    // 注册信号处理
    signal(SIGINT, SignalHandler);
    signal(SIGTERM, SignalHandler);
    signal(SIGPIPE, SIG_IGN);  // 忽略管道破裂信号
    
    std::cout << "Server is running. Press Ctrl+C to stop." << std::endl;
    
    // 启动服务器 (阻塞)
    server.Start();
    
    std::cout << "Server stopped." << std::endl;
    return 0;
}
