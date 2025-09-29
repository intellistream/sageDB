#!/bin/bash

# 简化的多模态模块测试脚本

set -e

echo "=== SAGE DB 多模态融合模块语法检查 ==="

cd /home/shuhao/SAGE/packages/sage-middleware/src/sage/middleware/components/sage_db

echo "检查头文件语法..."

# 检查多模态融合头文件
g++ -std=c++17 -I include -fsyntax-only include/sage_db/multimodal_fusion.h -DSYNTAX_CHECK
echo "✓ multimodal_fusion.h 语法正确"

g++ -std=c++17 -I include -fsyntax-only include/sage_db/fusion_strategies.h -DSYNTAX_CHECK  
echo "✓ fusion_strategies.h 语法正确"

g++ -std=c++17 -I include -fsyntax-only include/sage_db/multimodal_sage_db.h -DSYNTAX_CHECK
echo "✓ multimodal_sage_db.h 语法正确"

echo ""
echo "检查源文件语法..."

# 检查源文件（需要处理依赖）
echo "检查 fusion_strategies.cpp..."
g++ -std=c++17 -I include -c src/fusion_strategies.cpp -o /tmp/fusion_strategies.o
echo "✓ fusion_strategies.cpp 编译成功"

echo "检查 modality_manager.cpp..."  
g++ -std=c++17 -I include -c src/modality_manager.cpp -o /tmp/modality_manager.o
echo "✓ modality_manager.cpp 编译成功"

echo ""
echo "✅ 所有文件语法检查通过！"
echo ""
echo "模块结构:"
echo "📁 include/sage_db/"
find include/sage_db/ -name "*.h" | sort | sed 's/^/  📄 /'
echo ""
echo "📁 src/"
find src/ -name "*.cpp" | sort | sed 's/^/  📄 /'
echo ""
echo "📁 tests/"
find tests/ -name "*.cpp" | sort | sed 's/^/  📄 /'
echo ""
echo "📁 examples/"
find examples/ -name "*.cpp" | sort | sed 's/^/  📄 /'

echo ""
echo "下一步:"
echo "1. 修复任何编译依赖问题"
echo "2. 运行完整构建: ./build_multimodal.sh"
echo "3. 运行测试验证功能"