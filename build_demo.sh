#!/bin/bash

# Build script for multimodal demo
# This script compiles the multimodal data fusion demonstration

echo "🔨 Building SAGE DB Multimodal Fusion Demo..."

# Set directories
SAGE_DB_DIR="/home/shuhao/SAGE/packages/sage-middleware/src/sage/middleware/components/sage_db"
INCLUDE_DIR="$SAGE_DB_DIR/include"
SRC_DIR="$SAGE_DB_DIR/src"
EXAMPLES_DIR="$SAGE_DB_DIR/examples"

# Create build directory
mkdir -p "$SAGE_DB_DIR/build/examples"
cd "$SAGE_DB_DIR/build/examples"

echo "📁 Compiling source files..."

# Compile all required source files
g++ -std=c++17 -I "$INCLUDE_DIR" -c \
    "$SRC_DIR/sage_db.cpp" \
    "$SRC_DIR/vector_store.cpp" \
    "$SRC_DIR/metadata_store.cpp" \
    "$SRC_DIR/query_engine.cpp" \
    "$SRC_DIR/multimodal_sage_db.cpp" \
    "$SRC_DIR/fusion_strategies.cpp" \
    "$SRC_DIR/modality_manager.cpp" \
    "$EXAMPLES_DIR/multimodal_demo.cpp" \
    -D_GNU_SOURCE

if [ $? -ne 0 ]; then
    echo "❌ Compilation failed!"
    exit 1
fi

echo "🔗 Linking executable..."

# Link the executable
g++ -std=c++17 \
    sage_db.o \
    vector_store.o \
    metadata_store.o \
    query_engine.o \
    multimodal_sage_db.o \
    fusion_strategies.o \
    modality_manager.o \
    multimodal_demo.o \
    -o multimodal_demo \
    -lpthread

if [ $? -ne 0 ]; then
    echo "❌ Linking failed!"
    exit 1
fi

echo "✅ Build successful!"
echo "🚀 Run the demo with: ./multimodal_demo"
echo "📍 Demo location: $SAGE_DB_DIR/build/examples/multimodal_demo"