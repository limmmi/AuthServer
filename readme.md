# AuthServer - 认证服务器

一个高性能、极简设计的用户认证服务，基于 C++11 实现。

## 概述

独立运行的用户认证服务，负责：
- 用户注册
- 用户登录
- 签发 JWT 令牌

业务服务器无需连接本服务，本地使用公钥验签即可。

## 目录

- [特性](#特性)
- [技术栈](#技术栈)
- [项目结构](#项目结构)
- [快速开始](#快速开始)
- [API 接口](#api-接口)
- [核心设计](#核心设计)
- [安全规则](#安全规则)
- [相关文档](#相关文档)

## 特性

- **单线程 Reactor 模型**：基于 epoll 的非阻塞 IO，支持 5000+ 并发连接
- **JWT + RSA256**：非对称加密签发令牌，业务服务可离线验签
- **bcrypt 密码哈希**：不可逆加密存储密码
- **LevelDB 键值存储**：无需独立部署，开箱即用
- **完善的日志系统**：基于 spdlog，支持控制台和文件双输出
- **静态链接部署**：无运行时依赖，部署简单

## 技术栈

| 组件 | 技术 | 说明 |
|------|------|------|
| 开发语言 | C++11 | 现代 C++ 标准 |
| 网络模型 | 单线程 Reactor (epoll) | 高并发、无锁、稳定 |
| 数据库 | LevelDB | Google 开源键值数据库，静态链接 |
| 密码加密 | bcrypt | 不可逆哈希算法 |
| 令牌签发 | JWT + RSA256 | 非对称加密，私钥签名 |
| 日志系统 | spdlog | header-only，高性能 |
| 构建工具 | CMake | 跨平台构建 |

## 项目结构

```
AuthServer/
├── main.cpp              # 程序入口
├── AuthService.h/cpp     # 核心认证类
├── Database.h/cpp        # LevelDB 封装
├── JWT.h/cpp             # RSA 签发 JWT
├── Crypto.h/cpp          # bcrypt 密码哈希
├── Server.h/cpp          # 单线程 epoll 网络服务
├── Logger.h/cpp          # 日志系统封装
├── private.key           # RSA 私钥（外部文件，不进代码）
├── public.key            # RSA 公钥（可分发给业务服务）
├── CMakeLists.txt        # 编译脚本
├── build.sh              # 一键构建脚本
├── test_api.sh           # API 测试脚本
├── build/                # 编译输出目录
├── logs/                 # 日志文件目录
├── auth_db/              # LevelDB 数据库目录
├── spdlog/               # spdlog 源码（header-only）
├── leveldb_install/      # LevelDB 静态库
├── readme.md             # 本文档
├── 安装说明.md           # 详细安装步骤
├── 日志说明.md           # 日志系统文档
├── 数据库说明.md         # 数据库设计文档
├── 程序流程.md           # 程序流程文档
└── HTTP结构体说明.md     # HTTP 协议文档
```

## 快速开始

### 1. 安装依赖

```bash
sudo apt update
sudo apt install -y build-essential cmake libssl-dev
```

### 2. 编译 LevelDB

```bash
git clone https://github.com/google/leveldb.git
cd leveldb

cmake -B build -S . \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=../leveldb_install \
    -DLEVELDB_BUILD_TESTS=OFF \
    -DLEVELDB_BUILD_BENCHMARKS=OFF \
    -DCMAKE_CXX_FLAGS="-fexceptions -frtti"

cd build
make -j$(nproc)
make install
```

### 3. 下载 spdlog

```bash
cd /path/to/AuthServer
git clone --depth 1 https://github.com/gabime/spdlog.git
```

### 4. 生成 RSA 密钥

```bash
./generate_keys.sh
```

### 5. 编译项目

```bash
mkdir -p build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

或使用一键脚本：

```bash
./build.sh
```

### 6. 运行服务

```bash
./build/auth_server
# 或指定参数
./build/auth_server -p 8081 -d ./auth_db -k ./private.key
```

### 7. 测试 API

```bash
./test_api.sh
```

## API 接口

### 用户注册

```bash
POST /api/register
Content-Type: application/json

{
    "username": "alice",
    "password": "secret123"
}
```

响应：
```json
{
    "success": true,
    "message": "User registered successfully",
    "user_id": 1,
    "role": "normal"
}
```

### 用户登录

```bash
POST /api/login
Content-Type: application/json

{
    "username": "alice",
    "password": "secret123"
}
```

响应：
```json
{
    "success": true,
    "token": "eyJhbGciOiJSUzI1NiIs...",
    "user_id": 1,
    "role": "normal"
}
```

### 管理员注册（需管理员 Token）

```bash
POST /api/admin/register
Content-Type: application/json
X-Admin-Token: <admin_jwt_token>

{
    "username": "bob",
    "password": "admin123",
    "role": "admin"
}
```

## 核心设计

### 为什么选择单线程？

- 登录请求极少，不需要多线程
- 无锁、无竞争、稳定、不崩溃
- 代码简单、安全、易维护
- C++ 单线程性能足够支撑中小项目

### 数据库设计

| 键 | 值 |
|----|----|
| `user:{username}` | 密码哈希 + 用户 ID + 角色 |
| `id:{user_id}` | 用户名（反向索引） |
| `next_user_id` | 下一个用户 ID 计数器 |

### JWT 令牌结构

```json
{
    "header": {
        "alg": "RS256",
        "typ": "JWT"
    },
    "payload": {
        "sub": "1",
        "username": "alice",
        "role": "normal",
        "iat": 1705315200,
        "exp": 1705318800
    }
}
```

## 安全规则

- **私钥绝不放入代码**：从外部文件加载
- **密码绝不存明文**：必须 bcrypt 哈希
- **业务服务不连数据库**：只验签 JWT
- **公钥可以公开**：可硬编码在业务服务
- **JWT 自带过期**：自动失效，无需服务端状态

## 运行环境

- **操作系统**：Linux（Ubuntu 20.04+ / CentOS 7+）
- **运行方式**：后台守护进程（无界面）
- **默认端口**：8081（可配置）
- **并发能力**：5000+ 同时在线，1000+ QPS

## 相关文档

| 文档 | 说明 |
|------|------|
| [安装说明.md](./安装说明.md) | 详细的安装和编译步骤 |
| [日志说明.md](./日志说明.md) | 日志系统配置和使用 |
| [数据库说明.md](./数据库说明.md) | 数据库设计和操作说明 |
| [程序流程.md](./程序流程.md) | 程序执行流程详解 |
| [HTTP结构体说明.md](./HTTP结构体说明.md) | HTTP 协议和数据结构 |

