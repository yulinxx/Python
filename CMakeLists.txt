cmake_minimum_required(VERSION 4.3)

# ============================================================================
# PythonHost —— 独立构建入口
# ============================================================================
# 集成构建：根 CMakeLists.txt → add_subdirectory(PythonHost/PythonHost)（内层）。
# 独立构建：本文件提供 project() 与环境配置，把 PythonHost 单独拉出来开发 / 跑测试。
#
# 三条设计约束同时成立：
#
#   1) 复用仓库统一配置，不自己手搓一套，避免与主工程漂移：
#        ../Config.cmake               —— vcpkg / Qt 路径、MSVC 运行库、模块开关
#        ../CMake/StandaloneInit.cmake —— SANYI_ROOT、编译选项、GTest 查找、兄弟模块拉入
#      与 Log / Network / Hardware / Utility 的独立入口保持同一套写法。
#
#   2) 可脱离主程序单独编译：依赖的兄弟 DLL（Log，内层 PRIVATE 链接）由本工程
#      按需 add_subdirectory 拉进来构建；Main / UI / Engine 一概不参与。
#      运行期的 Python 解释器由宿主环境提供，构建期只编译框架本身。
#
#   3) 测试由内层 PythonHost/PythonHost/CMakeLists.txt 在
#      BUILD_TESTS=ON 时挂载，本文件不得重复 add_subdirectory(Test)。
#
# 用法：
#   cmake -S PythonHost -B PythonHost/build
#   cmake --build PythonHost/build --config Release
#   ctest --test-dir PythonHost/build -C Release
#   cmake -S PythonHost -B PythonHost/build -DBUILD_TESTS=OFF  # 只编库
# ============================================================================

include(${CMAKE_CURRENT_LIST_DIR}/../Config.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/../CMake/StandaloneInit.cmake)

project(SanYiPythonHost
    VERSION 1.0.0
    DESCRIPTION "SanYi CAD Python Host Integration Framework"
    LANGUAGES CXX
)

# MSVC 的 /W4 等选项必须在 project() 之后才能正确判定编译器
sanyi_setup_compiler_options()

# 示例目标仅存在于独立构建（内层不负责 Example）
option(BUILD_PYTHONHOST_EXAMPLE "Build PythonHost usage example" ON)

# --- 测试与兄弟模块 ---------------------------------------------------------
# 先声明本构建树的测试模块范围、再探测 GTest（缺失时把唯一总开关降级为 OFF），
# 两步都必须发生在拉入兄弟模块之前。
sanyi_only_own_tests(PYTHONHOST)
sanyi_finalize_tests()

# PythonHost 只依赖 Log（内层 PRIVATE 链接诊断输出）。
sanyi_add_sibling(Log "${SANYI_LOG_DIR}")

# 内层：创建 PythonHost DLL、挂载测试（BUILD_TESTS 时）、输出状态
add_subdirectory(PythonHost)

# --- 示例（独立构建专属） ---------------------------------------------------
if(BUILD_PYTHONHOST_EXAMPLE)
    add_subdirectory(Example)
endif()

# --- 独立构建摘要 -----------------------------------------------------------
message(STATUS "")
message(STATUS "========================================")
message(STATUS "  [Standalone: ${PROJECT_NAME}]")
message(STATUS "========================================")
message(STATUS "  Version:        ${PROJECT_VERSION}")
message(STATUS "  Build Example:  ${BUILD_PYTHONHOST_EXAMPLE}")
message(STATUS "  Output:         ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "========================================")
message(STATUS "")
