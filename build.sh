#!/bin/bash

# 编译脚本

set -e

echo "Building Auth Server..."

# 创建构建目录
mkdir -p build
cd build

# 运行 CMake
echo "Running CMake..."
cmake .. -DCMAKE_BUILD_TYPE=Release

# 编译
echo "Compiling..."
make -j$(nproc)

echo ""
echo "========================================"
echo "Build completed successfully!"
echo "========================================"
echo ""
echo "Binary location: build/auth_server"
echo ""
echo "To run the server:"
echo "  1. Generate RSA keys: ./generate_keys.sh"
echo "  2. Run server: ./build/auth_server"
echo ""
