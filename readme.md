认证服务器设计文档（极简、可直接落地、C++ 实现）
一、程序定位
独立运行的用户认证服务，负责：
用户注册
用户登录
签发 JWT 令牌
业务服务器不连接本服务，本地用公钥验签即可。
二、运行环境
操作系统：Linux（CentOS / Ubuntu 均可）
运行方式：后台守护进程（无界面）
端口：8081（可配置）
网络模型：单线程 + epoll 非阻塞 IO
并发能力：支持 5000+ 同时在线，每秒 1000+ 登录请求（完全够用）
三、核心技术选型
开发语言：C++11
网络模型：单线程 Reactor（epoll）
数据库：LevelDB（键值数据库，以动态库链接）
密码加密：bcrypt 哈希（不可逆加密）
令牌签发：JWT + RSA256 非对称加密
私钥：从外部文件加载
公钥：发给业务服务器（业务服务可硬编码）
四、单线程 vs 多线程（最终选择）
选择：单线程
理由：
登录请求极少，不需要多线程
无锁、无竞争、稳定、不崩溃
代码简单、安全、易维护
C++ 单线程性能足够支撑中小项目
五、数据库选型
LevelDB（Google 开源键值数据库）
以 动态库（libleveldb.so） 方式链接
不需要独立部署，开箱即用
存储结构：
key：用户名
value：密码哈希 + 用户 ID
业务服务器永远不连接数据库。
六、文件夹结构（极简标准）
plaintext
auth_server/
├── main.cpp              # 程序入口
├── AuthService.h/cpp      # 核心认证类
├── Database.h/cpp        # LevelDB 封装（动态库调用）
├── JWT.h/cpp             # RSA 签发 JWT
├── Crypto.h/cpp           # bcrypt 密码哈希
├── Server.h/cpp           # 单线程 epoll 网络服务
├── private.key           # RSA 私钥（外部文件，不进代码）
├── build/                 # 编译目录
└── CMakeLists.txt         # 编译脚本
七、主要类设计（文字伪代码）
1. AuthService（核心认证类）
功能：注册、登录、签发令牌
plaintext
类：AuthService
私有变量：
    rsa_private_key 字符串    # 从文件加载的私钥
    db 数据库对象             # LevelDB 实例
    token_expire 整数         # 过期时间（秒）

公有函数：
    构造函数(私钥路径, 过期秒数)
        加载私钥
        初始化数据库

    登录(用户名, 密码) -> 返回 JWT
        从数据库取出密码哈希
        验证密码是否正确
        正确 → 生成JWT并返回
        错误 → 返回空

    注册(用户名, 密码) -> 返回成功/失败
        检查用户是否已存在
        密码做 bcrypt 哈希
        存入数据库
        返回成功

私有函数：
    加载私钥文件()
    生成JWT(用户ID)
2. Database（LevelDB 封装类）
功能：提供 LevelDB 存取（调用动态库）
plaintext
类：Database
私有变量：
    leveldb::DB* db

公有函数：
    打开数据库(路径)
    关闭数据库()
    存数据(key, value)
    取数据(key) -> value
    删除数据(key)
    判断key是否存在
3. JWT 类（RSA 签名）
功能：使用私钥签发 JWT
plaintext
类：JWT
公有函数：
    生成JWT(用户ID, 私钥, 过期时间) -> token字符串
4. Crypto 类（密码哈希）
功能：密码加密、验证
plaintext
类：Crypto
静态函数：
    密码哈希(明文密码) -> 哈希字符串
    验证密码(明文密码, 哈希) -> 正确/错误
5. Server 类（网络服务）
功能：单线程 epoll 服务器
plaintext
类：Server
私有变量：
    监听端口
    epoll 文件描述符
    AuthService* auth

公有函数：
    启动()
        创建socket
        绑定端口
        epoll创建
        循环监听事件
        收到请求 → 分发到 注册/登录 接口

私有函数：
    处理客户端请求()
    解析HTTP请求
    调用登录/注册
    返回JSON结果
八、程序运行流程（最简）
启动 → 加载 RSA 私钥
初始化 LevelDB
启动单线程 epoll 服务器
等待用户请求
注册 → 密码哈希 → 存数据库
登录 → 验证密码 → 签发 JWT
返回 JWT 给用户
九、安全规则（必须遵守）
私钥绝不放入代码，从外部文件加载
密码绝不存明文，必须 bcrypt 哈希
业务服务不连数据库
公钥可以公开、硬编码
JWT 自带过期，自动失效
十、总结（一句话）
单线程、Linux 运行、LevelDB 动态库、RSA 签发 JWT、业务服务无状态、架构安全简单。