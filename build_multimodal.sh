#!/bin/bash

# SAGE DB 多模态融合模块构建脚本

set -e

echo "=== SAGE DB 多模态融合模块构建 ==="

# 创建构建目录
BUILD_DIR="build_multimodal"
if [ -d "$BUILD_DIR" ]; then
    echo "清理现有构建目录..."
    rm -rf "$BUILD_DIR"
fi

mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR"

echo "配置CMake..."
cmake .. \
    -DCMAKE_BUILD_TYPE=Release \
    -DBUILD_TESTS=ON \
    -DBUILD_PYTHON_BINDINGS=OFF \
    -DENABLE_MULTIMODAL=ON \
    -DENABLE_OPENCV=OFF \
    -DENABLE_FFMPEG=OFF

echo "编译项目..."
make -j$(nproc)

echo "✅ 构建完成！"

# 如果示例存在，编译示例
if [ -f "../examples/multimodal_example.cpp" ]; then
    echo "编译多模态示例..."
    g++ -std=c++17 \
        -I../include \
        -L. \
        ../examples/multimodal_example.cpp \
        -lsage_db \
        -o multimodal_example
    
    echo "✅ 示例编译完成！"
    echo "运行示例: ./multimodal_example"
fi

echo ""
echo "构建产物:"
echo "  - libsage_db.so    (主库)"
echo "  - multimodal_example (示例程序)"
echo ""
echo "使用方法:"
echo "  1. export LD_LIBRARY_PATH=\$PWD:\$LD_LIBRARY_PATH"
echo "  2. ./multimodal_example"