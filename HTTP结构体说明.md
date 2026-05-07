# AuthServer HTTP 结构体说明文档

本文档详细说明项目中 HTTP 相关的数据结构，包括请求/响应结构体和各个字段的含义。

---

## 一、HTTP 请求结构体（HttpRequest）

### 1.1 结构体定义

```cpp
struct HttpRequest {
    std::string method;                           // HTTP 方法
    std::string path;                             // 请求路径
    std::map<std::string, std::string> headers;   // 请求头
    std::string body;                             // 请求体
    std::map<std::string, std::string> queryParams; // URL 查询参数
};
```

### 1.2 字段详解

| 字段 | 类型 | 说明 | 示例 |
|------|------|------|------|
| `method` | string | HTTP 请求方法 | `"GET"`, `"POST"` |
| `path` | string | 请求路径（不含参数） | `"/api/login"` |
| `headers` | map | HTTP 请求头键值对 | `{"Content-Type": "application/json"}` |
| `body` | string | 请求体内容（JSON 或表单） | `{"username":"admin"}` |
| `queryParams` | map | URL 查询参数 | `{"username": "zhangsan"}` |

### 1.3 实际数据示例

#### 示例 1：POST 登录请求

**原始 HTTP 报文**：
```http
POST /api/login HTTP/1.1\r\n
Host: localhost:8081\r\n
Content-Type: application/json\r\n
Content-Length: 42\r\n
\r\n
{"username":"admin","password":"123456"}
```

**解析后的 HttpRequest**：
```cpp
HttpRequest request;
request.method = "POST";
request.path = "/api/login";
request.headers = {
    {"Host", "localhost:8081"},
    {"Content-Type", "application/json"},
    {"Content-Length", "42"}
};
request.body = "{\"username\":\"admin\",\"password\":\"123456\"}";
request.queryParams = {};  // POST 请求无 URL 参数
```

#### 示例 2：GET 查询请求

**原始 HTTP 报文**：
```http
GET /api/user/info?username=zhangsan&detail=true HTTP/1.1\r\n
Host: localhost:8081\r\n
\r\n
```

**解析后的 HttpRequest**：
```cpp
HttpRequest request;
request.method = "GET";
request.path = "/api/user/info";
request.headers = {
    {"Host", "localhost:8081"}
};
request.body = "";  // GET 请求无 body
request.queryParams = {
    {"username", "zhangsan"},
    {"detail", "true"}
};
```

---

## 二、HTTP 响应结构体（HttpResponse）

### 2.1 结构体定义

```cpp
struct HttpResponse {
    int statusCode;                               // HTTP 状态码
    std::map<std::string, std::string> headers;   // 响应头
    std::string body;                             // 响应体
    
    // 构造函数设置默认值
    HttpResponse() : statusCode(200) {
        headers["Content-Type"] = "application/json";
        headers["Connection"] = "close";
    }
};
```

### 2.2 字段详解

| 字段 | 类型 | 说明 | 常用值 |
|------|------|------|--------|
| `statusCode` | int | HTTP 状态码 | `200`, `400`, `401`, `404`, `500` |
| `headers` | map | 响应头键值对 | `{"Content-Type": "application/json"}` |
| `body` | string | 响应体内容（JSON 格式） | `{"success":"true"}` |

### 2.3 状态码说明

| 状态码 | 含义 | 使用场景 |
|--------|------|----------|
| `200` | OK | 请求成功 |
| `400` | Bad Request | 参数缺失或格式错误 |
| `401` | Unauthorized | 认证失败（密码错误） |
| `403` | Forbidden | 权限不足（非管理员） |
| `404` | Not Found | 接口不存在或用户不存在 |
| `500` | Internal Server Error | 服务器内部错误 |

### 2.4 实际数据示例

#### 示例 1：登录成功响应

**HttpResponse 对象**：
```cpp
HttpResponse response;
response.statusCode = 200;
response.headers = {
    {"Content-Type", "application/json"},
    {"Connection", "close"}
};
response.body = "{\"success\":\"true\",\"token\":\"eyJhbG...\",\"role\":\"admin\"}";
```

**实际 HTTP 响应报文**：
```http
HTTP/1.1 200 OK\r\n
Content-Type: application/json\r\n
Connection: close\r\n
Content-Length: 85\r\n
\r\n
{"success":"true","token":"eyJhbG...","role":"admin"}
```

#### 示例 2：登录失败响应

**HttpResponse 对象**：
```cpp
HttpResponse response;
response.statusCode = 401;
response.body = "{\"error\":\"Invalid username or password\"}";
```

**实际 HTTP 响应报文**：
```http
HTTP/1.1 401 Unauthorized\r\n
Content-Type: application/json\r\n
Connection: close\r\n
Content-Length: 44\r\n
\r\n
{"error":"Invalid username or password"}
```

---

## 三、结构体方法详解

### 3.1 请求解析方法（Server::ParseHttpRequest）

**功能**：将原始 HTTP 报文字符串解析为 HttpRequest 结构体

**输入**：
```cpp
std::string rawRequest = 
    "POST /api/login HTTP/1.1\r\n"
    "Host: localhost:8081\r\n"
    "Content-Type: application/json\r\n"
    "Content-Length: 42\r\n"
    "\r\n"
    "{\"username\":\"admin\",\"password\":\"123456\"}";
```

**处理流程**：
```cpp
HttpRequest Server::ParseHttpRequest(const std::string& rawRequest) {
    HttpRequest request;
    
    // 步骤 1: 解析请求行
    // "POST /api/login HTTP/1.1" → method="POST", path="/api/login"
    
    // 步骤 2: 解析请求头
    // "Host: localhost:8081" → headers["Host"] = "localhost:8081"
    
    // 步骤 3: 解析请求体
    // 从 \r\n\r\n 之后提取 body
    
    // 步骤 4: 解析 URL 参数（GET 请求）
    // "/api/user/info?username=zhangsan" → queryParams["username"] = "zhangsan"
    
    return request;
}
```

**代码位置**：`Server.cpp:202-270`

---

### 3.2 响应构建方法（Server::BuildHttpResponse）

**功能**：将 HttpResponse 结构体转换为 HTTP 响应报文字符串

**输入**：
```cpp
HttpResponse response;
response.statusCode = 200;
response.headers["Content-Type"] = "application/json";
response.body = "{\"success\":\"true\"}";
```

**处理流程**：
```cpp
std::string Server::BuildHttpResponse(const HttpResponse& response) {
    std::stringstream ss;
    
    // 步骤 1: 写入状态行
    ss << "HTTP/1.1 " << response.statusCode << " ";
    switch (response.statusCode) {
        case 200: ss << "OK"; break;
        case 400: ss << "Bad Request"; break;
        // ... 其他状态码
    }
    ss << "\r\n";
    
    // 步骤 2: 写入响应头
    for (const auto& header : response.headers) {
        ss << header.first << ": " << header.second << "\r\n";
    }
    
    // 步骤 3: 写入 Content-Length
    ss << "Content-Length: " << response.body.length() << "\r\n";
    
    // 步骤 4: 空行分隔
    ss << "\r\n";
    
    // 步骤 5: 写入响应体
    ss << response.body;
    
    return ss.str();  // 返回完整 HTTP 响应字符串
}
```

**代码位置**：`Server.cpp:272-295`

---

### 3.3 请求路由方法（Server::RouteRequest）

**功能**：根据请求方法和路径分发到对应的处理函数

**路由表**：

| 方法 | 路径 | 处理函数 | 说明 |
|------|------|----------|------|
| POST | `/api/register` | `HandleRegister` | 用户注册 |
| POST | `/api/login` | `HandleLogin` | 用户登录 |
| POST | `/api/admin/register` | `HandleAdminRegister` | 管理员注册 |
| GET | `/api/user/info` | `HandleGetUserInfo` | 查询用户信息 |
| GET | `/api/health` | `HandleHealth` | 健康检查 |

**实现代码**：
```cpp
HttpResponse Server::RouteRequest(const HttpRequest& request) {
    if (request.method == "POST" && request.path == "/api/register") {
        return HandleRegister(request);
    } 
    else if (request.method == "POST" && request.path == "/api/login") {
        return HandleLogin(request);
    }
    else if (request.method == "POST" && request.path == "/api/admin/register") {
        return HandleAdminRegister(request);
    }
    else if (request.method == "GET" && request.path == "/api/user/info") {
        return HandleGetUserInfo(request);
    }
    else if (request.method == "GET" && request.path == "/api/health") {
        return HandleHealth(request);
    }
    
    // 404 未找到
    HttpResponse response;
    response.statusCode = 404;
    response.body = "{\"error\":\"Not Found\"}";
    return response;
}
```

**代码位置**：`Server.cpp:297-318`

---

## 四、各接口处理函数详解

### 4.1 HandleRegister - 用户注册

**输入 HttpRequest**：
```cpp
request.method = "POST";
request.path = "/api/register";
request.body = "{\"username\":\"zhangsan\",\"password\":\"123456\"}";
```

**处理流程**：
```cpp
HttpResponse Server::HandleRegister(const HttpRequest& request) {
    // 1. 解析 JSON 请求体
    auto params = ParseJson(request.body);
    // params = {"username": "zhangsan", "password": "123456"}
    
    // 2. 检查必填参数
    if (缺少参数) {
        return 400 Bad Request;
    }
    
    // 3. 调用业务逻辑
    bool success = authService_->Register(username, password);
    
    // 4. 构造响应
    if (success) {
        return 200 + {"success": "true", "message": "..."};
    } else {
        return 400 + {"error": "Registration failed"};
    }
}
```

**输出 HttpResponse**：
```cpp
// 成功
statusCode = 200;
body = "{\"success\":\"true\",\"message\":\"User registered successfully\"}";

// 失败
statusCode = 400;
body = "{\"error\":\"Registration failed (user may exist or invalid input)\"}";
```

---

### 4.2 HandleLogin - 用户登录

**输入 HttpRequest**：
```cpp
request.method = "POST";
request.path = "/api/login";
request.body = "{\"username\":\"admin\",\"password\":\"admin123\"}";
```

**处理流程**：
```cpp
HttpResponse Server::HandleLogin(const HttpRequest& request) {
    // 1. 解析 JSON
    auto params = ParseJson(request.body);
    
    // 2. 调用登录
    std::string token = authService_->Login(username, password);
    
    // 3. 构造响应
    if (token.empty()) {
        return 401 + {"error": "Invalid username or password"};
    } else {
        // 获取用户信息
        UserInfo info;
        authService_->GetUserInfo(username, info);
        
        return 200 + {
            "success": "true",
            "token": token,
            "role": "admin",
            "userId": "1"
        };
    }
}
```

**输出 HttpResponse**：
```cpp
// 成功
statusCode = 200;
body = R"({
    "success": "true",
    "token": "eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCJ9...",
    "role": "admin",
    "userId": "1"
})";

// 失败
statusCode = 401;
body = "{\"error\":\"Invalid username or password\"}";
```

---

### 4.3 HandleAdminRegister - 管理员注册

**输入 HttpRequest**：
```cpp
request.method = "POST";
request.path = "/api/admin/register";
request.body = R"({
    "username": "newuser",
    "password": "pass123",
    "role": "normal",
    "admin_token": "eyJhbG..."
})";
```

**处理流程**：
```cpp
HttpResponse Server::HandleAdminRegister(const HttpRequest& request) {
    // 1. 解析所有参数
    auto params = ParseJson(request.body);
    // params = {
    //     "username": "newuser",
    //     "password": "pass123",
    //     "role": "normal",
    //     "admin_token": "eyJhbG..."
    // }
    
    // 2. 调用管理员注册（内部验证 token 权限）
    bool success = authService_->RegisterWithRole(
        username, password, role, admin_token
    );
    
    // 3. 构造响应
    if (success) {
        return 200 + {"success": "true", "role": "normal"};
    } else {
        return 403 + {"error": "Registration failed (not admin...)"};
    }
}
```

---

### 4.4 HandleGetUserInfo - 查询用户信息

**输入 HttpRequest**：
```cpp
request.method = "GET";
request.path = "/api/user/info";
request.queryParams = {"username": "zhangsan"};
request.body = "";  // GET 无 body
```

**处理流程**：
```cpp
HttpResponse Server::HandleGetUserInfo(const HttpRequest& request) {
    // 1. 从 URL 参数获取用户名
    auto it = request.queryParams.find("username");
    std::string username = it->second;  // "zhangsan"
    
    // 2. 查询用户信息
    UserInfo info;
    if (authService_->GetUserInfo(username, info)) {
        return 200 + {
            "username": "zhangsan",
            "userId": "2",
            "role": "normal"
        };
    } else {
        return 404 + {"error": "User not found"};
    }
}
```

---

### 4.5 HandleHealth - 健康检查

**输入 HttpRequest**：
```cpp
request.method = "GET";
request.path = "/api/health";
```

**处理流程**：
```cpp
HttpResponse Server::HandleHealth(const HttpRequest& request) {
    HttpResponse response;
    response.body = "{\"status\":\"ok\",\"service\":\"auth_server\"}";
    return response;  // 默认 200 OK
}
```

---

## 五、辅助方法详解

### 5.1 ParseJson - JSON 解析

**功能**：将 JSON 字符串解析为键值对 map

**输入**：
```cpp
std::string json = "{\"username\":\"admin\",\"password\":\"123456\"}";
```

**输出**：
```cpp
std::map<std::string, std::string> params = {
    {"username", "admin"},
    {"password", "123456"}
};
```

**代码位置**：`Server.cpp:520-550`

---

### 5.2 BuildJson - JSON 构建

**功能**：将键值对 map 转换为 JSON 字符串

**输入**：
```cpp
std::map<std::string, std::string> data = {
    {"success", "true"},
    {"token", "eyJhbG..."}
};
```

**输出**：
```cpp
std::string json = "{\"success\":\"true\",\"token\":\"eyJhbG...\"}";
```

**代码位置**：`Server.cpp:552-568`

---

### 5.3 UrlDecode - URL 解码

**功能**：将 URL 编码的字符串解码为原始字符串

**输入**：
```cpp
std::string encoded = "zhang%40example.com";  // %40 = @
```

**输出**：
```cpp
std::string decoded = "zhang@example.com";
```

**代码位置**：`Server.cpp:490-518`

---

## 六、完整请求-响应流程示例

### 场景：用户登录

```
┌─────────────┐                              ┌─────────────┐
│   客户端     │ ────── 1. 发送请求 ───────▶ │  AuthServer │
│  (curl/浏览器)│                              │             │
└─────────────┘                              └──────┬──────┘
                                                    │
                                                    ▼
┌─────────────────────────────────────────────────────────────┐
│  2. Server::HandleClientData() 接收数据                      │
│     └── 读取到完整 HTTP 请求                                 │
└─────────────────────────────────────────────────────────────┘
                                                    │
                                                    ▼
┌─────────────────────────────────────────────────────────────┐
│  3. Server::ParseHttpRequest() 解析                          │
│     └── HttpRequest {                                        │
│           method: "POST"                                     │
│           path: "/api/login"                                 │
│           body: "{\"username\":\"admin\",\"password\":\"123\"}"│
│         }                                                    │
└─────────────────────────────────────────────────────────────┘
                                                    │
                                                    ▼
┌─────────────────────────────────────────────────────────────┐
│  4. Server::RouteRequest() 路由                              │
│     └── 匹配到 HandleLogin()                                 │
└─────────────────────────────────────────────────────────────┘
                                                    │
                                                    ▼
┌─────────────────────────────────────────────────────────────┐
│  5. Server::HandleLogin() 处理                               │
│     ├── ParseJson() → {username, password}                  │
│     ├── authService_->Login() → 验证并生成 token            │
│     └── 构造 HttpResponse                                    │
│           statusCode: 200                                    │
│           body: {success, token, role, userId}              │
└─────────────────────────────────────────────────────────────┘
                                                    │
                                                    ▼
┌─────────────────────────────────────────────────────────────┐
│  6. Server::BuildHttpResponse() 构建响应                     │
│     └── "HTTP/1.1 200 OK\r\nContent-Type:...\r\n\r\n{...}"   │
└─────────────────────────────────────────────────────────────┘
                                                    │
                                                    ▼
┌─────────────┐                              ┌─────────────┐
│   客户端     │ ◀───── 7. 接收响应 ───────── │  AuthServer │
│             │                              │             │
└─────────────┘                              └─────────────┘
```

---

## 七、总结

| 结构体/方法 | 作用 | 关键字段/操作 |
|-------------|------|---------------|
| `HttpRequest` | 封装 HTTP 请求数据 | method, path, headers, body, queryParams |
| `HttpResponse` | 封装 HTTP 响应数据 | statusCode, headers, body |
| `ParseHttpRequest` | 解析原始 HTTP 报文 | 提取请求行、头、体、参数 |
| `BuildHttpResponse` | 构建 HTTP 响应报文 | 组装状态行、头、体 |
| `RouteRequest` | 请求路由分发 | 根据 method+path 选择处理函数 |
| `HandleXxx` | 各接口业务处理 | 调用 AuthService，构造响应 |
| `ParseJson/BuildJson` | JSON 序列化/反序列化 | map ↔ JSON 字符串 |
