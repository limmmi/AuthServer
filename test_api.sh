#!/bin/bash

# API 测试脚本

BASE_URL="http://localhost:8081"

echo "========================================"
echo "       Auth Server API Test           "
echo "========================================"
echo ""

# 测试健康检查
echo "1. Testing health check..."
curl -s -X GET "${BASE_URL}/api/health"
echo ""
echo ""

# 测试注册
echo "2. Testing user registration..."
curl -s -X POST "${BASE_URL}/api/register" \
    -H "Content-Type: application/json" \
    -d '{"username":"testuser","password":"testpass123"}'
echo ""
echo ""

# 测试重复注册 (应该失败)
echo "3. Testing duplicate registration (should fail)..."
curl -s -X POST "${BASE_URL}/api/register" \
    -H "Content-Type: application/json" \
    -d '{"username":"testuser","password":"testpass123"}'
echo ""
echo ""

# 测试登录
echo "4. Testing user login..."
RESPONSE=$(curl -s -X POST "${BASE_URL}/api/login" \
    -H "Content-Type: application/json" \
    -d '{"username":"testuser","password":"testpass123"}')
echo "$RESPONSE"
echo ""

# 提取 token
TOKEN=$(echo "$RESPONSE" | grep -o '"token":"[^"]*"' | cut -d'"' -f4)
if [ ! -z "$TOKEN" ]; then
    echo ""
    echo "Extracted JWT Token:"
    echo "$TOKEN"
    echo ""
    
    # 解码 JWT payload (简化版)
    echo "5. Decoding JWT payload..."
    PAYLOAD=$(echo "$TOKEN" | cut -d'.' -f2)
    # 添加填充
    PADDED=$(echo "$PAYLOAD" | tr '_-' '/+' | awk '{print $0"==="}')
    echo "$PADDED" | base64 -d 2>/dev/null || echo "(raw: $PAYLOAD)"
    echo ""
fi

# 测试错误密码
echo "6. Testing login with wrong password (should fail)..."
curl -s -X POST "${BASE_URL}/api/login" \
    -H "Content-Type: application/json" \
    -d '{"username":"testuser","password":"wrongpassword"}'
echo ""
echo ""

# 测试不存在的用户
echo "7. Testing login with non-existent user (should fail)..."
curl -s -X POST "${BASE_URL}/api/login" \
    -H "Content-Type: application/json" \
    -d '{"username":"nonexistent","password":"somepass"}'
echo ""
echo ""

echo "========================================"
echo "           Test Completed             "
echo "========================================"
