cmake_minimum_required(VERSION 4.3)

# --- 模块元信息 ---
set(LIB_NAME "PythonHost")
set(LIB_FOLDER "Python")
set(LIB_DESCRIPTION "Python Host Integration Framework")
set(LIB_EXPORT_MACRO "PYTHONHOST_EXPORTS")

# --- 版本（按模块独立）---
set(VERSION_MAJOR 1)
set(VERSION_MINOR 0)
set(VERSION_PATCH 0)
set(PROJECT_VERSION "${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_PATCH}")

# --- 路径 ---
if(NOT APR_ROOT)
    get_filename_component(APR_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/../.." ABSOLUTE)
endif()
include("${APR_ROOT}/CMake/AprPaths.cmake")
include("${APR_ROOT}/CMake/Utils.cmake")

# --- 源文件 ---
file(GLOB_RECURSE LIB_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/Src/*.c"
)
file(GLOB_RECURSE LIB_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.h"
    "${CMAKE_CURRENT_SOURCE_DIR}/Include/*.hpp"
)
list(FILTER LIB_SOURCES EXCLUDE REGEX "${APR_SOURCE_EXCLUDE_REGEX}")
list(FILTER LIB_HEADERS EXCLUDE REGEX "${APR_HEADER_EXCLUDE_REGEX}")

# --- 统一入口创建 DLL ---
apr_add_shared_library(${LIB_NAME}
    FOLDER "${LIB_FOLDER}"
    DESCRIPTION "${LIB_DESCRIPTION}"
    EXPORT_MACRO "${LIB_EXPORT_MACRO}"
    VERSION_MAJOR ${VERSION_MAJOR}
    VERSION_MINOR ${VERSION_MINOR}
    VERSION_PATCH ${VERSION_PATCH}
    SOURCES ${LIB_SOURCES}
    HEADERS ${LIB_HEADERS}
    PUBLIC_INCLUDE_DIRS
        "${CMAKE_CURRENT_SOURCE_DIR}/Include"
    PRIVATE_INCLUDE_DIRS
        "${APR_LOG_DIR}/Include"
    PRIVATE_LINK_LIBRARIES
        Log
    PRIVATE_QT_COMPONENTS
        Core
)

# --- 单元测试 ---
# 由唯一总开关 BUILD_TESTS 与当前构建范围控制（统一挂载点，独立入口不得重复）。
apr_module_tests_enabled(PYTHONHOST _pythonhost_tests)
if(_pythonhost_tests)
    add_subdirectory(Test)
endif()