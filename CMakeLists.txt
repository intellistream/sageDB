cmake_minimum_required(VERSION 3.12)
project(sage_db VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build options
option(BUILD_TESTS "Build test programs" ON)
option(BUILD_PYTHON_BINDINGS "Build Python bindings" ON)
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

# Python bindings
if(BUILD_PYTHON_BINDINGS)
    # Use same pattern as sage_flow - let pybind11 handle Python detection
    # find_package(Python3 COMPONENTS Interpreter Development REQUIRED)

    set(_sage_db_deps_included FALSE)
    if(DEFINED SAGE_COMMON_DEPS_FILE AND EXISTS "${SAGE_COMMON_DEPS_FILE}")
        include("${SAGE_COMMON_DEPS_FILE}")
        set(_sage_db_deps_included TRUE)
    endif()

    if(NOT _sage_db_deps_included)
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/pybind11_dependency.cmake)
    endif()

    if(ENABLE_GPERFTOOLS AND NOT (DEFINED SAGE_GPERFTOOLS_LIBS AND SAGE_GPERFTOOLS_LIBS))
        include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/gperftools.cmake)
    endif()

    pybind11_add_module(_sage_db python/bindings.cpp)
    target_link_libraries(_sage_db PRIVATE sage_db)
    target_compile_definitions(_sage_db PRIVATE VERSION_INFO="${PROJECT_VERSION}")

    if(FAISS_FOUND)
        target_compile_definitions(_sage_db PRIVATE ENABLE_FAISS)
        target_include_directories(_sage_db PRIVATE ${FAISS_INCLUDE_DIR})
        target_link_libraries(_sage_db PRIVATE ${FAISS_LIBRARY})
    endif()

    if(DEFINED SAGE_COMMON_COMPILE_DEFINITIONS)
        target_compile_definitions(_sage_db PRIVATE ${SAGE_COMMON_COMPILE_DEFINITIONS})
    else()
        target_compile_definitions(_sage_db PRIVATE PYBIND11_INTERNALS_ID="sage_pybind11_shared")
    endif()

    if(DEFINED SAGE_COMMON_COMPILE_OPTIONS)
        target_compile_options(_sage_db PRIVATE ${SAGE_COMMON_COMPILE_OPTIONS})
    else()
        # Minimize exported symbols (align with _sage_flow) to avoid cross-extension symbol collisions
        target_compile_options(_sage_db PRIVATE -fvisibility=hidden -fvisibility-inlines-hidden)
    endif()

    if(ENABLE_GPERFTOOLS)
        if(DEFINED SAGE_GPERFTOOLS_LIBS AND SAGE_GPERFTOOLS_LIBS)
            target_link_libraries(sage_db PRIVATE ${SAGE_GPERFTOOLS_LIBS})
            target_link_libraries(_sage_db PRIVATE ${SAGE_GPERFTOOLS_LIBS})
            target_compile_definitions(sage_db PRIVATE ENABLE_GPERFTOOLS)
            target_compile_definitions(_sage_db PRIVATE ENABLE_GPERFTOOLS)
        else()
            message(WARNING "ENABLE_GPERFTOOLS requested but gperftools libraries were not found; disabling support")
            set(ENABLE_GPERFTOOLS OFF CACHE BOOL "Enable gperftools profiling support" FORCE)
        endif()
    endif()
    set_target_properties(_sage_db PROPERTIES CXX_VISIBILITY_PRESET hidden VISIBILITY_INLINES_HIDDEN YES)
    
    # 禁用 LTO 以避免链接器问题，设置输出属性
    set_target_properties(_sage_db PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION OFF
        OUTPUT_NAME "_sage_db"
        PREFIX ""
        # 设置RPATH以便Python模块能找到共享库
        BUILD_RPATH "${CMAKE_CURRENT_BINARY_DIR};${CMAKE_INSTALL_PREFIX}/lib"
        INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;$ORIGIN/../build;$ORIGIN/../install/lib;$ORIGIN/../../install/lib"
        INSTALL_RPATH_USE_LINK_PATH TRUE
    )
    
    # 在某些系统上，pybind11 默认启用 LTO，我们需要显式禁用
    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(_sage_db PRIVATE -fno-lto)
        target_link_options(_sage_db PRIVATE -fno-lto)
    endif()
endif()

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

# Install rules
install(TARGETS sage_db
    EXPORT sage_db_targets
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    RUNTIME DESTINATION bin
)

install(DIRECTORY include/ DESTINATION include)

if(BUILD_PYTHON_BINDINGS)
    install(TARGETS _sage_db DESTINATION .)
endif()
