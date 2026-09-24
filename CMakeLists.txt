cmake_minimum_required(VERSION 4.3)

add_subdirectory(PythonHost)

# 单元测试（开关由 Config.cmake 统一配置）
if(BUILD_PYTHONHOST_TESTS)
    enable_testing()
    add_subdirectory(PythonHost/Test)
endif()
