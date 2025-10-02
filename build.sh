#!/bin/bash
set -e

BUILD_TYPE=${BUILD_TYPE:-Debug}

echo "Building SAGE DB with CMake (CMAKE_BUILD_TYPE=${BUILD_TYPE})..."

# Function to check and fix libstdc++ version issue in conda environment
check_libstdcxx() {
    # Only check if we're in a conda environment
    if [[ -z "${CONDA_PREFIX}" ]]; then
        return 0
    fi
    
    # Check if conda libstdc++ needs update
    local conda_libstdcxx="${CONDA_PREFIX}/lib/libstdc++.so.6"
    if [[ ! -f "${conda_libstdcxx}" ]]; then
        return 0
    fi
    
    # Check GCC version requirement
    local gcc_version=$(gcc -dumpversion | cut -d. -f1)
    if [[ ${gcc_version} -ge 11 ]]; then
        # Check if conda libstdc++ has required GLIBCXX version
        if ! strings "${conda_libstdcxx}" | grep -q "GLIBCXX_3.4.30"; then
            echo "⚠️  检测到conda环境中的libstdc++版本过低，正在更新..."
            echo "   这是C++20/GCC 11+编译所必需的"
            
            # Try to update libstdc++ in conda environment
            if command -v conda &> /dev/null; then
                conda install -c conda-forge libstdcxx-ng -y || {
                    echo "⚠️  无法自动更新libstdc++，将使用系统版本"
                    # Set LD_LIBRARY_PATH to prefer system libstdc++
                    if [[ -f "/usr/lib/x86_64-linux-gnu/libstdc++.so.6" ]]; then
                        export LD_LIBRARY_PATH="/usr/lib/x86_64-linux-gnu:${LD_LIBRARY_PATH}"
                        echo "   已设置LD_LIBRARY_PATH优先使用系统libstdc++"
                    fi
                }
            fi
        fi
    fi
}

# Check libstdc++ before building
check_libstdcxx

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

echo "SAGE DB build completed."