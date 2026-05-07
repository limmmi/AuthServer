#!/bin/bash

# 生成 RSA 密钥对脚本

echo "Generating RSA key pair for Auth Server..."

# 生成私钥 (2048位)
openssl genrsa -out private.key 2048
if [ $? -ne 0 ]; then
    echo "Failed to generate private key"
    exit 1
fi

# 从私钥提取公钥
openssl rsa -in private.key -pubout -out public.key
if [ $? -ne 0 ]; then
    echo "Failed to generate public key"
    exit 1
fi

echo ""
echo "========================================"
echo "RSA key pair generated successfully!"
echo "========================================"
echo ""
echo "Files:"
echo "  private.key - Keep this SECRET (used by auth server)"
echo "  public.key  - Can be distributed to business servers"
echo ""
echo "To use in your business server (C++ example):"
echo ""
cat public.key
echo ""
echo "========================================"
