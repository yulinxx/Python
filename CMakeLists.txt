cmake_minimum_required(VERSION 4.3)

add_subdirectory(PythonHost)

# 单元测试
if(BUILD_ALL_TESTS OR BUILD_PYTHONHOST_TESTS)
    enable_testing()
    add_subdirectory(PythonHost/Test)
endif()
