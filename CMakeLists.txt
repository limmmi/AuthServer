cmake_minimum_required(VERSION 3.10)
project(auth_server)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# 查找必要的库
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

set(LEVELDB_ROOT "${CMAKE_SOURCE_DIR}/leveldb_install")

# 使用静态库
set(LEVELDB_STATIC_LIB "${LEVELDB_ROOT}/lib/libleveldb.a")
if(NOT EXISTS ${LEVELDB_STATIC_LIB})
    message(FATAL_ERROR "LevelDB static library not found: ${LEVELDB_STATIC_LIB}")
endif()
message(STATUS "Using LevelDB static library: ${LEVELDB_STATIC_LIB}")

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${LEVELDB_ROOT}/include
    ${CMAKE_SOURCE_DIR}/spdlog/include    # 添加 spdlog 头文件路径
)

# 源文件
set(SOURCES
    main.cpp
    AuthService.cpp
    Database.cpp
    JWT.cpp
    Crypto.cpp
    Server.cpp
    Logger.cpp
)

# 生成可执行文件
add_executable(auth_server ${SOURCES})

# 链接库（静态链接 LevelDB）
target_link_libraries(auth_server
    ${LEVELDB_STATIC_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# 安装目标
install(TARGETS auth_server DESTINATION bin)
