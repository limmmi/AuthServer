cmake_minimum_required(VERSION 3.10)
project(auth_server)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -O2")

# 查找必要的库
find_package(OpenSSL REQUIRED)
find_package(Threads REQUIRED)

# LevelDB 动态库 (使用本地编译的版本)
set(LEVELDB_ROOT "${CMAKE_SOURCE_DIR}/leveldb_install")
find_library(LEVELDB_LIB leveldb 
    PATHS ${LEVELDB_ROOT}/lib
    NO_DEFAULT_PATH
)
if(NOT LEVELDB_LIB)
    message(FATAL_ERROR "LevelDB library not found in ${LEVELDB_ROOT}/lib")
endif()

include_directories(
    ${CMAKE_SOURCE_DIR}
    ${LEVELDB_ROOT}/include
)

# 源文件
set(SOURCES
    main.cpp
    AuthService.cpp
    Database.cpp
    JWT.cpp
    Crypto.cpp
    Server.cpp
)

# 生成可执行文件
add_executable(auth_server ${SOURCES})

# 链接库
target_link_libraries(auth_server
    ${LEVELDB_LIB}
    OpenSSL::SSL
    OpenSSL::Crypto
    Threads::Threads
)

# 安装目标
install(TARGETS auth_server DESTINATION bin)
