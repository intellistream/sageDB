#!/bin/bash
set -e

BUILD_TYPE=${BUILD_TYPE:-Debug}

echo "Building SAGE DB with CMake (CMAKE_BUILD_TYPE=${BUILD_TYPE})..."

# Install dependencies if needed
install_deps() {
    echo "Installing dependencies..."
    
    # Install Python dependencies
    echo "📦 Installing Python dependencies..."
    if command -v pip3 &> /dev/null; then
        echo "📦 Trying: pip install faiss-cpu pybind11[global]"
        pip3 install faiss-cpu pybind11[global] || echo "⚠️ pip install failed, continuing..."
    fi
    
    # Install system dependencies in non-CI environments
    if [[ "${CI:-false}" != "true" ]]; then
        if command -v apt-get &> /dev/null; then
            echo "🔧 Installing BLAS/LAPACK via apt..."
            sudo apt-get update -qq && sudo apt-get install -y libopenblas-dev liblapack-dev || echo "⚠️ apt install failed"
        elif command -v brew &> /dev/null; then
            echo "🔧 Installing BLAS/LAPACK via homebrew..."
            brew install openblas lapack || echo "⚠️ brew install failed"
        fi
    fi
}

# Parse arguments
for arg in "$@"; do
    case $arg in
        --install-deps)
            install_deps
            shift
            ;;
        --debug)
            BUILD_TYPE="Debug"
            shift
            ;;
        --clean)
            echo "Cleaning previous build..."
            rm -rf build install
            shift
            ;;
    esac
done

# Create build directory if not exists
mkdir -p build

# Configure with CMake (same pattern as sage_flow)
cmake_args=(
    -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
    -DCMAKE_INSTALL_PREFIX="$(pwd)/install"
    -DBUILD_TESTS=ON
    -DBUILD_PYTHON_BINDINGS=ON
    -DUSE_OPENMP=ON
)

# Add common SAGE environment variables (same as sage_flow)
if [[ -n "${SAGE_COMMON_DEPS_FILE:-}" ]]; then
    cmake_args+=(-DSAGE_COMMON_DEPS_FILE="${SAGE_COMMON_DEPS_FILE}")
fi
if [[ -n "${SAGE_ENABLE_GPERFTOOLS:-}" ]]; then
    cmake_args+=(-DSAGE_ENABLE_GPERFTOOLS="${SAGE_ENABLE_GPERFTOOLS}")
fi
if [[ -n "${SAGE_PYBIND11_VERSION:-}" ]]; then
    cmake_args+=(-DSAGE_PYBIND11_VERSION="${SAGE_PYBIND11_VERSION}")
fi
if [[ -n "${SAGE_GPERFTOOLS_ROOT:-}" ]]; then
    cmake_args+=(-DSAGE_GPERFTOOLS_ROOT="${SAGE_GPERFTOOLS_ROOT}")
fi

# Configure with CMake
cmake -B build "${cmake_args[@]}"

# Build (same as sage_flow)
cmake --build build -j "$(nproc)"

# Copy Python extension for import stability (keep this for compatibility)
if [[ -f "build/_sage_db"*.so ]]; then
    echo "Copying Python extension into python/ ..."
    mkdir -p python
    cp build/_sage_db*.so python/
fi

echo "SAGE DB build completed."