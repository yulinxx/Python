/**
 * @file PythonHostTypesTests.cpp
 * @brief PythonHost 类型定义单元测试
 *
 * 覆盖:
 * - TaskError 枚举值
 * - TaskMode 枚举值
 * - TaskPriority 枚举值
 * - TaskRequest / TaskResult / TaskDefinition 结构体
 * - Config / RuntimeInfo 结构体
 */
#include <gtest/gtest.h>

#include "PythonHost/PythonHostTypes.h"

using namespace PyHost;

TEST(PythonHostTypesTest, TaskError_EnumValues)
{
    // 验证所有 TaskError 枚举值存在且可转换
    EXPECT_EQ(static_cast<int>(TaskError::None), 0);
    EXPECT_EQ(static_cast<int>(TaskError::PythonNotFound), 1);
    EXPECT_EQ(static_cast<int>(TaskError::ScriptFailed), 2);
    EXPECT_EQ(static_cast<int>(TaskError::Timeout), 3);
    EXPECT_EQ(static_cast<int>(TaskError::Cancelled), 4);
    EXPECT_EQ(static_cast<int>(TaskError::InvalidInput), 5);
    EXPECT_EQ(static_cast<int>(TaskError::ProtocolError), 6);
    EXPECT_EQ(static_cast<int>(TaskError::UnknownTask), 7);
    EXPECT_EQ(static_cast<int>(TaskError::NotInitialized), 8);
}

TEST(PythonHostTypesTest, TaskMode_EnumValues)
{
    EXPECT_EQ(static_cast<int>(TaskMode::Subprocess), 0);
    EXPECT_EQ(static_cast<int>(TaskMode::Bridge), 1);
}

TEST(PythonHostTypesTest, TaskPriority_EnumValues)
{
    EXPECT_EQ(static_cast<int>(TaskPriority::Low), 0);
    EXPECT_EQ(static_cast<int>(TaskPriority::Normal), 1);
    EXPECT_EQ(static_cast<int>(TaskPriority::High), 2);
}

TEST(PythonHostTypesTest, TaskRequest_DefaultValues)
{
    TaskRequest request;

    EXPECT_EQ(request.taskId, "");
    EXPECT_EQ(request.inputJson, "{}");
    EXPECT_EQ(request.traceId, "");
    EXPECT_EQ(request.timeoutMs, 60000);
    EXPECT_TRUE(request.cancellable);
    EXPECT_EQ(request.priority, TaskPriority::Normal);
}

TEST(PythonHostTypesTest, TaskRequest_SetValues)
{
    TaskRequest request;
    request.taskId = "test_task";
    request.inputJson = R"({"key": "value"})";
    request.traceId = "trace_123";
    request.timeoutMs = 30000;
    request.cancellable = false;
    request.priority = TaskPriority::High;

    EXPECT_EQ(request.taskId, "test_task");
    EXPECT_EQ(request.inputJson, R"({"key": "value"})");
    EXPECT_EQ(request.traceId, "trace_123");
    EXPECT_EQ(request.timeoutMs, 30000);
    EXPECT_FALSE(request.cancellable);
    EXPECT_EQ(request.priority, TaskPriority::High);
}

TEST(PythonHostTypesTest, TaskResult_DefaultValues)
{
    TaskResult result;

    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.error, TaskError::None);
    EXPECT_EQ(result.message, "");
    EXPECT_EQ(result.resultJson, "{}");
    EXPECT_EQ(result.metricsJson, "{}");
    EXPECT_EQ(result.handle, 0);
    EXPECT_EQ(result.taskId, "");
    EXPECT_EQ(result.traceId, "");
}

TEST(PythonHostTypesTest, TaskResult_SetValues)
{
    TaskResult result;
    result.ok = true;
    result.error = TaskError::ScriptFailed;
    result.message = "Error message";
    result.resultJson = R"({"output": "data"})";
    result.metricsJson = R"({"time": 100})";
    result.handle = 12345;
    result.taskId = "task_123";
    result.traceId = "trace_abc";

    EXPECT_TRUE(result.ok);
    EXPECT_EQ(result.error, TaskError::ScriptFailed);
    EXPECT_EQ(result.message, "Error message");
    EXPECT_EQ(result.resultJson, R"({"output": "data"})");
    EXPECT_EQ(result.metricsJson, R"({"time": 100})");
    EXPECT_EQ(result.handle, 12345);
    EXPECT_EQ(result.taskId, "task_123");
    EXPECT_EQ(result.traceId, "trace_abc");
}

TEST(PythonHostTypesTest, TaskDefinition_DefaultValues)
{
    TaskDefinition definition;

    EXPECT_EQ(definition.taskId, "");
    EXPECT_EQ(definition.entry, "");
    EXPECT_EQ(definition.mode, TaskMode::Subprocess);
    EXPECT_EQ(definition.timeoutMs, 60000);
}

TEST(PythonHostTypesTest, TaskDefinition_SetValues)
{
    TaskDefinition definition;
    definition.taskId = "my_task";
    definition.entry = "scripts/my_task.py";
    definition.mode = TaskMode::Bridge;
    definition.timeoutMs = 30000;

    EXPECT_EQ(definition.taskId, "my_task");
    EXPECT_EQ(definition.entry, "scripts/my_task.py");
    EXPECT_EQ(definition.mode, TaskMode::Bridge);
    EXPECT_EQ(definition.timeoutMs, 30000);
}

TEST(PythonHostTypesTest, Config_DefaultValues)
{
    Config config;

    EXPECT_EQ(config.pythonRoot, "");
    EXPECT_EQ(config.venvPath, "");
    EXPECT_EQ(config.extensionPath, "");
    EXPECT_EQ(config.tasksConfigPath, "");
    EXPECT_EQ(config.sourcePythonRoot, "");
    EXPECT_FALSE(config.enableEmbedded);
    EXPECT_TRUE(config.dispatchCallbacksOnMainThread);
    EXPECT_EQ(config.workerThreads, 2);
}

TEST(PythonHostTypesTest, Config_SetValues)
{
    Config config;
    config.pythonRoot = "/usr/bin/python3";
    config.venvPath = "/opt/venv";
    config.extensionPath = "./extensions";
    config.tasksConfigPath = "config/tasks.json";
    config.sourcePythonRoot = "/usr/local/python";
    config.enableEmbedded = true;
    config.dispatchCallbacksOnMainThread = false;
    config.workerThreads = 4;

    EXPECT_EQ(config.pythonRoot, "/usr/bin/python3");
    EXPECT_EQ(config.venvPath, "/opt/venv");
    EXPECT_EQ(config.extensionPath, "./extensions");
    EXPECT_EQ(config.tasksConfigPath, "config/tasks.json");
    EXPECT_EQ(config.sourcePythonRoot, "/usr/local/python");
    EXPECT_TRUE(config.enableEmbedded);
    EXPECT_FALSE(config.dispatchCallbacksOnMainThread);
    EXPECT_EQ(config.workerThreads, 4);
}

TEST(PythonHostTypesTest, RuntimeInfo_DefaultValues)
{
    RuntimeInfo info;

    EXPECT_FALSE(info.initialized);
    EXPECT_EQ(info.pythonExecutable, "");
    EXPECT_EQ(info.pythonRoot, "");
    EXPECT_EQ(info.pythonVersion, "");
    EXPECT_EQ(info.registeredTaskCount, 0);
}

TEST(PythonHostTypesTest, RuntimeInfo_SetValues)
{
    RuntimeInfo info;
    info.initialized = true;
    info.pythonExecutable = "/usr/bin/python3";
    info.pythonRoot = "/usr";
    info.pythonVersion = "3.11.0";
    info.registeredTaskCount = 10;

    EXPECT_TRUE(info.initialized);
    EXPECT_EQ(info.pythonExecutable, "/usr/bin/python3");
    EXPECT_EQ(info.pythonRoot, "/usr");
    EXPECT_EQ(info.pythonVersion, "3.11.0");
    EXPECT_EQ(info.registeredTaskCount, 10);
}

TEST(PythonHostTypesTest, TypeAliases)
{
    // 验证类型别名
    TaskHandle handle = 12345;
    JsonString json = R"({"key": "value"})";

    EXPECT_EQ(handle, 12345);
    EXPECT_EQ(json, R"({"key": "value"})");

    // TaskCallback 类型
    TaskCallback callback = [](const TaskResult& result) {
        // 空回调
    };

    // 回调应该可以被调用（验证类型正确）
    TaskResult dummyResult;
    EXPECT_NO_THROW(callback(dummyResult));
}
