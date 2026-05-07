#include "Server.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <iostream>
#include <sstream>
#include <cctype>

Server::Server(int port, AuthService* authService)
    : port_(port)
    , listenFd_(-1)
    , epollFd_(-1)
    , authService_(authService)
    , running_(false) {
}

Server::~Server() {
    Stop();
}

bool Server::SetNonBlocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

bool Server::CreateListenSocket() {
    // 创建 socket
    listenFd_ = socket(AF_INET, SOCK_STREAM, 0);
    if (listenFd_ == -1) {
        std::cerr << "Failed to create socket" << std::endl;
        return false;
    }
    
    // 设置地址重用
    int reuse = 1;
    if (setsockopt(listenFd_, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) == -1) {
        std::cerr << "Failed to set SO_REUSEADDR" << std::endl;
        close(listenFd_);
        return false;
    }
    
    // 设置非阻塞
    if (!SetNonBlocking(listenFd_)) {
        std::cerr << "Failed to set non-blocking" << std::endl;
        close(listenFd_);
        return false;
    }
    
    // 绑定地址
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port_);
    
    if (bind(listenFd_, (struct sockaddr*)&addr, sizeof(addr)) == -1) {
        std::cerr << "Failed to bind to port " << port_ << std::endl;
        close(listenFd_);
        return false;
    }
    
    // 开始监听
    if (listen(listenFd_, 128) == -1) {
        std::cerr << "Failed to listen" << std::endl;
        close(listenFd_);
        return false;
    }
    
    std::cout << "Server listening on port " << port_ << std::endl;
    return true;
}

void Server::Start() {
    if (!CreateListenSocket()) {
        return;
    }
    
    // 创建 epoll 实例
    epollFd_ = epoll_create1(0);
    if (epollFd_ == -1) {
        std::cerr << "Failed to create epoll" << std::endl;
        return;
    }
    
    // 添加监听 socket 到 epoll
    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.fd = listenFd_;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, listenFd_, &ev) == -1) {
        std::cerr << "Failed to add listen socket to epoll" << std::endl;
        return;
    }
    
    running_ = true;
    struct epoll_event events[MAX_EVENTS];
    
    std::cout << "Server started, waiting for connections..." << std::endl;
    
    while (running_) {
        int nfds = epoll_wait(epollFd_, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            if (errno == EINTR) continue;
            std::cerr << "epoll_wait error" << std::endl;
            break;
        }
        
        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == listenFd_) {
                // 新连接
                HandleNewConnection();
            } else {
                // 客户端数据
                HandleClientData(events[i].data.fd);
            }
        }
    }
}

void Server::Stop() {
    running_ = false;
    
    // 关闭所有客户端连接
    for (const auto& pair : clientBuffers_) {
        close(pair.first);
    }
    clientBuffers_.clear();
    
    if (epollFd_ != -1) {
        close(epollFd_);
        epollFd_ = -1;
    }
    
    if (listenFd_ != -1) {
        close(listenFd_);
        listenFd_ = -1;
    }
}

void Server::HandleNewConnection() {
    struct sockaddr_in clientAddr;
    socklen_t addrLen = sizeof(clientAddr);
    
    int clientFd = accept(listenFd_, (struct sockaddr*)&clientAddr, &addrLen);
    if (clientFd == -1) {
        if (errno != EAGAIN && errno != EWOULDBLOCK) {
            std::cerr << "accept error" << std::endl;
        }
        return;
    }
    
    // 设置非阻塞
    if (!SetNonBlocking(clientFd)) {
        close(clientFd);
        return;
    }
    
    // 添加到 epoll
    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;  // 边缘触发
    ev.data.fd = clientFd;
    if (epoll_ctl(epollFd_, EPOLL_CTL_ADD, clientFd, &ev) == -1) {
        close(clientFd);
        return;
    }
    
    clientBuffers_[clientFd] = "";
}

void Server::HandleClientData(int clientFd) {
    char buffer[BUFFER_SIZE];
    
    while (true) {
        ssize_t n = read(clientFd, buffer, sizeof(buffer) - 1);
        
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // 数据读取完毕
                break;
            }
            // 错误
            CloseClient(clientFd);
            return;
        } else if (n == 0) {
            // 客户端关闭连接
            CloseClient(clientFd);
            return;
        }
        
        buffer[n] = '\0';
        clientBuffers_[clientFd] += buffer;
    }
    
    // 检查是否收到完整 HTTP 请求
    std::string& data = clientBuffers_[clientFd];
    size_t headerEnd = data.find("\r\n\r\n");
    
    if (headerEnd != std::string::npos) {
        // 解析 Content-Length
        size_t contentLength = 0;
        size_t clPos = data.find("Content-Length: ");
        if (clPos != std::string::npos) {
            size_t clEnd = data.find("\r\n", clPos);
            contentLength = std::stoul(data.substr(clPos + 16, clEnd - clPos - 16));
        }
        
        // 检查是否收到完整 body
        if (data.length() >= headerEnd + 4 + contentLength) {
            // 处理请求
            HttpRequest request = ParseHttpRequest(data);
            HttpResponse response = RouteRequest(request);
            
            // 发送响应
            std::string responseStr = BuildHttpResponse(response);
            write(clientFd, responseStr.data(), responseStr.length());
            
            // 关闭连接 (HTTP/1.0 短连接)
            CloseClient(clientFd);
        }
    }
}

void Server::CloseClient(int clientFd) {
    epoll_ctl(epollFd_, EPOLL_CTL_DEL, clientFd, nullptr);
    close(clientFd);
    clientBuffers_.erase(clientFd);
}

HttpRequest Server::ParseHttpRequest(const std::string& rawRequest) {
    HttpRequest request;
    
    // 解析请求行
    size_t lineEnd = rawRequest.find("\r\n");
    if (lineEnd == std::string::npos) return request;
    
    std::string requestLine = rawRequest.substr(0, lineEnd);
    size_t firstSpace = requestLine.find(' ');
    size_t secondSpace = requestLine.find(' ', firstSpace + 1);
    
    if (firstSpace != std::string::npos && secondSpace != std::string::npos) {
        request.method = requestLine.substr(0, firstSpace);
        std::string fullPath = requestLine.substr(firstSpace + 1, secondSpace - firstSpace - 1);
        
        // 分离路径和查询参数
        size_t qPos = fullPath.find('?');
        if (qPos != std::string::npos) {
            request.path = fullPath.substr(0, qPos);
            std::string query = fullPath.substr(qPos + 1);
            
            // 解析查询参数
            size_t start = 0;
            while (start < query.length()) {
                size_t eqPos = query.find('=', start);
                size_t ampPos = query.find('&', start);
                if (eqPos == std::string::npos) break;
                
                std::string key = query.substr(start, eqPos - start);
                std::string value = (ampPos == std::string::npos) 
                    ? query.substr(eqPos + 1)
                    : query.substr(eqPos + 1, ampPos - eqPos - 1);
                
                request.queryParams[key] = UrlDecode(value);
                start = (ampPos == std::string::npos) ? query.length() : ampPos + 1;
            }
        } else {
            request.path = fullPath;
        }
    }
    
    // 解析头部
    size_t headerStart = lineEnd + 2;
    size_t headerEnd = rawRequest.find("\r\n\r\n", headerStart);
    
    if (headerEnd != std::string::npos) {
        std::string headers = rawRequest.substr(headerStart, headerEnd - headerStart);
        size_t pos = 0;
        
        while (pos < headers.length()) {
            size_t lineEnd = headers.find("\r\n", pos);
            if (lineEnd == std::string::npos) lineEnd = headers.length();
            
            std::string line = headers.substr(pos, lineEnd - pos);
            size_t colonPos = line.find(':');
            
            if (colonPos != std::string::npos) {
                std::string key = line.substr(0, colonPos);
                std::string value = line.substr(colonPos + 1);
                // 去除前导空格
                size_t valueStart = value.find_first_not_of(" \t");
                if (valueStart != std::string::npos) {
                    value = value.substr(valueStart);
                }
                request.headers[key] = value;
            }
            
            pos = lineEnd + 2;
        }
        
        // 解析 body
        request.body = rawRequest.substr(headerEnd + 4);
    }
    
    return request;
}

std::string Server::BuildHttpResponse(const HttpResponse& response) {
    std::stringstream ss;
    
    ss << "HTTP/1.1 " << response.statusCode << " ";
    switch (response.statusCode) {
        case 200: ss << "OK"; break;
        case 400: ss << "Bad Request"; break;
        case 401: ss << "Unauthorized"; break;
        case 404: ss << "Not Found"; break;
        case 500: ss << "Internal Server Error"; break;
        default: ss << "Unknown"; break;
    }
    ss << "\r\n";
    
    for (const auto& header : response.headers) {
        ss << header.first << ": " << header.second << "\r\n";
    }
    
    ss << "Content-Length: " << response.body.length() << "\r\n";
    ss << "\r\n";
    ss << response.body;
    
    return ss.str();
}

HttpResponse Server::RouteRequest(const HttpRequest& request) {
    if (request.method == "POST" && request.path == "/api/login") {
        return HandleLogin(request);
    } else if (request.method == "POST" && request.path == "/api/register") {
        return HandleRegister(request);
    } else if (request.method == "POST" && request.path == "/api/admin/register") {
        return HandleAdminRegister(request);
    } else if (request.method == "GET" && request.path == "/api/user/info") {
        return HandleGetUserInfo(request);
    } else if (request.method == "GET" && request.path == "/api/health") {
        return HandleHealth(request);
    }
    
    HttpResponse response;
    response.statusCode = 404;
    response.body = BuildJson({{"error", "Not Found"}});
    return response;
}

HttpResponse Server::HandleLogin(const HttpRequest& request) {
    HttpResponse response;
    
    auto params = ParseJson(request.body);
    auto itUsername = params.find("username");
    auto itPassword = params.find("password");
    
    if (itUsername == params.end() || itPassword == params.end()) {
        response.statusCode = 400;
        response.body = BuildJson({{"error", "Missing username or password"}});
        return response;
    }
    
    std::string token = authService_->Login(itUsername->second, itPassword->second);
    
    if (token.empty()) {
        response.statusCode = 401;
        response.body = BuildJson({{"error", "Invalid username or password"}});
    } else {
        // 获取用户角色信息
        UserInfo info;
        authService_->GetUserInfo(itUsername->second, info);
        
        response.body = BuildJson({
            {"success", "true"},
            {"token", token},
            {"role", AuthService::RoleToString(info.role)},
            {"userId", info.userId}
        });
    }
    
    return response;
}

HttpResponse Server::HandleRegister(const HttpRequest& request) {
    HttpResponse response;
    
    auto params = ParseJson(request.body);
    auto itUsername = params.find("username");
    auto itPassword = params.find("password");
    
    if (itUsername == params.end() || itPassword == params.end()) {
        response.statusCode = 400;
        response.body = BuildJson({{"error", "Missing username or password"}});
        return response;
    }
    
    if (authService_->Register(itUsername->second, itPassword->second)) {
        response.body = BuildJson({
            {"success", "true"},
            {"message", "User registered successfully"}
        });
    } else {
        response.statusCode = 400;
        response.body = BuildJson({{"error", "Registration failed (user may exist or invalid input)"}});
    }
    
    return response;
}

HttpResponse Server::HandleHealth(const HttpRequest& request) {
    HttpResponse response;
    response.body = BuildJson({
        {"status", "ok"},
        {"service", "auth_server"}
    });
    return response;
}

HttpResponse Server::HandleAdminRegister(const HttpRequest& request) {
    HttpResponse response;
    
    auto params = ParseJson(request.body);
    auto itUsername = params.find("username");
    auto itPassword = params.find("password");
    auto itRole = params.find("role");
    auto itToken = params.find("admin_token");
    
    if (itUsername == params.end() || itPassword == params.end() || 
        itRole == params.end() || itToken == params.end()) {
        response.statusCode = 400;
        response.body = BuildJson({{"error", "Missing username, password, role or admin_token"}});
        return response;
    }
    
    UserRole role = (itRole->second == "admin") ? UserRole::ADMIN : UserRole::NORMAL;
    
    if (authService_->RegisterWithRole(itUsername->second, itPassword->second, 
                                       role, itToken->second)) {
        response.body = BuildJson({
            {"success", "true"},
            {"message", "User registered successfully"},
            {"role", itRole->second}
        });
    } else {
        response.statusCode = 403;
        response.body = BuildJson({{"error", "Registration failed (not admin, user exists or invalid input)"}});
    }
    
    return response;
}

HttpResponse Server::HandleGetUserInfo(const HttpRequest& request) {
    HttpResponse response;
    
    auto it = request.queryParams.find("username");
    if (it == request.queryParams.end()) {
        response.statusCode = 400;
        response.body = BuildJson({{"error", "Missing username parameter"}});
        return response;
    }
    
    UserInfo info;
    if (authService_->GetUserInfo(it->second, info)) {
        response.body = BuildJson({
            {"username", it->second},
            {"userId", info.userId},
            {"role", AuthService::RoleToString(info.role)}
        });
    } else {
        response.statusCode = 404;
        response.body = BuildJson({{"error", "User not found"}});
    }
    
    return response;
}

std::string Server::UrlDecode(const std::string& encoded) {
    std::string result;
    result.reserve(encoded.length());
    
    for (size_t i = 0; i < encoded.length(); ++i) {
        if (encoded[i] == '%' && i + 2 < encoded.length()) {
            int hex1 = std::tolower(encoded[i + 1]);
            int hex2 = std::tolower(encoded[i + 2]);
            
            int value = 0;
            if (hex1 >= '0' && hex1 <= '9') value = (hex1 - '0') << 4;
            else if (hex1 >= 'a' && hex1 <= 'f') value = (hex1 - 'a' + 10) << 4;
            
            if (hex2 >= '0' && hex2 <= '9') value |= (hex2 - '0');
            else if (hex2 >= 'a' && hex2 <= 'f') value |= (hex2 - 'a' + 10);
            
            result += static_cast<char>(value);
            i += 2;
        } else if (encoded[i] == '+') {
            result += ' ';
        } else {
            result += encoded[i];
        }
    }
    
    return result;
}

std::map<std::string, std::string> Server::ParseJson(const std::string& json) {
    std::map<std::string, std::string> result;
    
    size_t pos = 0;
    while ((pos = json.find('"', pos)) != std::string::npos) {
        size_t keyEnd = json.find('"', pos + 1);
        if (keyEnd == std::string::npos) break;
        
        std::string key = json.substr(pos + 1, keyEnd - pos - 1);
        
        size_t colonPos = json.find(':', keyEnd);
        if (colonPos == std::string::npos) break;
        
        size_t valueStart = json.find_first_not_of(" \t", colonPos + 1);
        if (valueStart == std::string::npos) break;
        
        std::string value;
        if (json[valueStart] == '\"') {
            size_t valueEnd = json.find('"', valueStart + 1);
            if (valueEnd == std::string::npos) break;
            value = json.substr(valueStart + 1, valueEnd - valueStart - 1);
            pos = valueEnd + 1;
        } else {
            size_t valueEnd = json.find_first_of(",}", valueStart);
            value = json.substr(valueStart, valueEnd - valueStart);
            pos = valueEnd;
        }
        
        result[key] = value;
    }
    
    return result;
}

std::string Server::BuildJson(const std::map<std::string, std::string>& data) {
    std::stringstream ss;
    ss << "{";
    
    bool first = true;
    for (const auto& pair : data) {
        if (!first) ss << ",";
        first = false;
        
        ss << "\"" << pair.first << "\":\"" << pair.second << "\"";
    }
    
    ss << "}";
    return ss.str();
}
