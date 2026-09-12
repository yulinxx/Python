# PythonHost 单元测试
#
# 测试覆盖:
# - TaskRegistry: 任务注册表
# - RuntimeManager: 运行时管理器
# - PythonHostTypes: 类型定义

find_package(GTest CONFIG REQUIRED)
include(GoogleTest)

file(GLOB TEST_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/*.cpp")

# 复用 PythonHost 库的源文件进行白盒测试
file(GLOB_RECURSE MODULE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../Src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Src/*.cxx"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Src/*.cc"
)

add_executable(PythonHostTests ${TEST_SOURCES} ${MODULE_SOURCES})

target_include_directories(PythonHostTests PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../Include"
    "${CMAKE_CURRENT_SOURCE_DIR}/../Src"
)

target_link_libraries(PythonHostTests PRIVATE
    GTest::gtest
    GTest::gtest_main
    Qt6::Core
)

target_compile_features(PythonHostTests PRIVATE cxx_std_17)

if(MSVC)
    target_compile_options(PythonHostTests PRIVATE /utf-8 /FS)
endif()

set_target_properties(PythonHostTests PROPERTIES FOLDER "Tests")

gtest_discover_tests(PythonHostTests
    DISCOVERY_TIMEOUT 60
)

message(STATUS "  [PythonHost] PythonHostTests: enabled")
