cmake_minimum_required(VERSION 3.12)
project(sage_db VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build test programs" ON)
option(USE_OPENMP "Enable OpenMP support" ON)
option(ENABLE_MULTIMODAL "Enable multimodal fusion support" ON)
option(ENABLE_OPENCV "Enable OpenCV for image processing" OFF)
option(ENABLE_FFMPEG "Enable FFmpeg for audio/video processing" OFF)

set(_sage_db_enable_gperftools_default OFF)
if(DEFINED SAGE_ENABLE_GPERFTOOLS)
    set(_sage_db_enable_gperftools_default ${SAGE_ENABLE_GPERFTOOLS})
endif()
option(ENABLE_GPERFTOOLS "Enable gperftools profiling support" ${_sage_db_enable_gperftools_default})
if(DEFINED SAGE_ENABLE_GPERFTOOLS)
    set(ENABLE_GPERFTOOLS ${SAGE_ENABLE_GPERFTOOLS} CACHE BOOL "Enable gperftools profiling support" FORCE)
endif()

# Find packages (keep minimal to improve portability)

# Add cmake modules directory to the path
list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")

# Use enhanced BLAS/LAPACK finder
include(FindBLASLAPACK)

# Try to find FAISS
find_path(FAISS_INCLUDE_DIR NAMES faiss/IndexFlat.h
    HINTS
    ${FAISS_ROOT}/include
    /usr/local/include
    /usr/include
    /opt/conda/include
    $ENV{CONDA_PREFIX}/include
)

find_library(FAISS_LIBRARY NAMES faiss
    HINTS
    ${FAISS_ROOT}/lib
    /usr/local/lib
    /usr/lib
    /opt/conda/lib
    $ENV{CONDA_PREFIX}/lib
)

if(FAISS_INCLUDE_DIR AND FAISS_LIBRARY)
    message(STATUS "Found FAISS: ${FAISS_LIBRARY}")
    set(FAISS_FOUND TRUE)
else()
    message(STATUS "FAISS C++ library not found, using fallback implementation")
    message(STATUS "Note: Python faiss package should be available for full functionality")
    set(FAISS_FOUND FALSE)
endif()

# OpenMP support
if(USE_OPENMP)
    find_package(OpenMP)
    if(OpenMP_CXX_FOUND)
        message(STATUS "OpenMP found")
    endif()
endif()

# Source files
set(SAGE_DB_SOURCES
    src/sage_db.cpp
    src/vector_store.cpp
    src/metadata_store.cpp
    src/query_engine.cpp
    src/anns/anns_interface.cpp
    src/anns/brute_force_plugin.cpp
)

set(SAGE_DB_HEADERS
    include/sage_db/sage_db.h
    include/sage_db/vector_store.h
    include/sage_db/metadata_store.h
    include/sage_db/query_engine.h
    include/sage_db/common.h
    include/sage_db/anns/anns_interface.h
    include/sage_db/anns/brute_force_plugin.h
)

if(FAISS_FOUND)
    list(APPEND SAGE_DB_SOURCES src/anns/faiss_plugin.cpp)
    list(APPEND SAGE_DB_HEADERS include/sage_db/anns/faiss_plugin.h)
endif()

# Multimodal fusion sources and headers
if(ENABLE_MULTIMODAL)
    list(APPEND SAGE_DB_SOURCES
        src/multimodal_sage_db.cpp
        src/fusion_strategies.cpp
        src/modality_manager.cpp
    )
    
    list(APPEND SAGE_DB_HEADERS
        include/sage_db/multimodal_fusion.h
        include/sage_db/multimodal_sage_db.h
        include/sage_db/fusion_strategies.h
    )
endif()

# Create shared library
add_library(sage_db SHARED ${SAGE_DB_SOURCES})

# Include directories
target_include_directories(sage_db PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

if(FAISS_FOUND)
    target_include_directories(sage_db PRIVATE ${FAISS_INCLUDE_DIR})
    target_link_libraries(sage_db PRIVATE ${FAISS_LIBRARY})
    if(HAVE_BLAS_LAPACK)
        target_link_libraries(sage_db PRIVATE ${BLAS_LIBRARIES} ${LAPACK_LIBRARIES})
    endif()
    target_compile_definitions(sage_db PRIVATE ENABLE_FAISS)
endif()

# Multimodal fusion dependencies
if(ENABLE_MULTIMODAL)
    target_compile_definitions(sage_db PRIVATE MULTIMODAL_ENABLED)
    
    # OpenCV for image processing
    if(ENABLE_OPENCV)
        find_package(OpenCV REQUIRED)
        target_include_directories(sage_db PRIVATE ${OpenCV_INCLUDE_DIRS})
        target_link_libraries(sage_db PRIVATE ${OpenCV_LIBS})
        target_compile_definitions(sage_db PRIVATE OPENCV_ENABLED)
    endif()
    
    # FFmpeg for audio/video processing  
    if(ENABLE_FFMPEG)
        find_package(PkgConfig)
        if(PKG_CONFIG_FOUND)
            pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)
            target_include_directories(sage_db PRIVATE ${FFMPEG_INCLUDE_DIRS})
            target_link_libraries(sage_db PRIVATE ${FFMPEG_LIBRARIES})
            target_compile_definitions(sage_db PRIVATE FFMPEG_ENABLED)
        endif()
    endif()
endif()

# OpenMP linking
if(OpenMP_CXX_FOUND)
    target_link_libraries(sage_db PUBLIC OpenMP::OpenMP_CXX)
endif()

# Compiler specific options
target_compile_options(sage_db PRIVATE
    $<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -O3>
    $<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -O3>
    $<$<CXX_COMPILER_ID:MSVC>:/W4 /O2>
)

# Tests
if(BUILD_TESTS)
    enable_testing()
    
    add_executable(test_sage_db tests/test_sage_db.cpp)
    target_link_libraries(test_sage_db PRIVATE sage_db)
    target_include_directories(test_sage_db PRIVATE include)
    
    add_test(NAME test_sage_db COMMAND test_sage_db)
    
    # Multimodal tests
    if(ENABLE_MULTIMODAL)
        add_executable(test_multimodal tests/test_multimodal.cpp)
        target_link_libraries(test_multimodal PRIVATE sage_db)
        target_include_directories(test_multimodal PRIVATE include)
        
        add_test(NAME test_multimodal COMMAND test_multimodal)
    endif()
endif()

# Install rules - 条件化安装以支持 SKBUILD 和独立构建
if(DEFINED SKBUILD)
    if(SKBUILD_STATE STREQUAL "editable")
        # Editable install: install to source tree directly
        set(_lib_install_dest "${CMAKE_CURRENT_SOURCE_DIR}/python")
    else()
        # Wheel build: install to wheel platlib (same directory as Python extension)
        set(_lib_install_dest "${SKBUILD_PLATLIB_DIR}/sage/middleware/components/sage_db/python")
    endif()
    
    install(TARGETS sage_db
        LIBRARY DESTINATION ${_lib_install_dest}
        ARCHIVE DESTINATION ${_lib_install_dest}
        RUNTIME DESTINATION ${_lib_install_dest}
        COMPONENT python
    )
else()
    # 独立 C++ 构建模式：标准安装路径
    install(TARGETS sage_db
        EXPORT sage_db_targets
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )
endif()

install(DIRECTORY include/ DESTINATION include)

# ============================================================================
# Python bindings (optional)
# ============================================================================
option(BUILD_PYTHON_BINDINGS "Build Python bindings" OFF)

if(BUILD_PYTHON_BINDINGS)
    message(STATUS "Building Python bindings for sage_db")
    add_subdirectory(python)
endif()
