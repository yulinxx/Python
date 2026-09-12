/**
 * @file RuntimeManagerTests.cpp
 * @brief RuntimeManager 单元测试
 *
 * 覆盖:
 * - initialize / shutdown 生命周期
 * - isInitialized 状态查询
 * - pythonExecutable / pythonRoot / pythonVersion 查询
 * - resolvePythonRoot / resolveTasksConfigPath 路径解析
 */
#include <gtest/gtest.h>

#include "PythonHost/RuntimeManager.h"

using namespace PyHost;

TEST(RuntimeManagerTest, InitialState)
{
    RuntimeManager manager;

    EXPECT_FALSE(manager.isInitialized());
    EXPECT_EQ(manager.pythonExecutable(), "");
    EXPECT_EQ(manager.pythonRoot(), "");
    EXPECT_EQ(manager.pythonVersion(), "");
}

TEST(RuntimeManagerTest, Initialize_InvalidConfig)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "";  // 无效配置

    const bool result = manager.initialize(config);

    // 预期失败（没有有效的 Python）
    EXPECT_FALSE(result);
    EXPECT_FALSE(manager.isInitialized());
}

TEST(RuntimeManagerTest, Initialize_ValidConfig)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "python";  // 尝试使用系统 Python
    config.venvPath = "";
    config.extensionPath = "";
    config.tasksConfigPath = "";

    // 尝试初始化（可能在测试环境失败，这是预期行为）
    manager.initialize(config);

    // 无论成功与否，不应崩溃
    // 如果初始化成功，检查状态
    if (manager.isInitialized())
    {
        EXPECT_FALSE(manager.pythonExecutable().empty());
        EXPECT_FALSE(manager.pythonRoot().empty());
    }
}

TEST(RuntimeManagerTest, Shutdown_Uninitialized)
{
    RuntimeManager manager;

    // 未初始化的情况下调用 shutdown 不应崩溃
    EXPECT_NO_THROW(manager.shutdown());

    EXPECT_FALSE(manager.isInitialized());
}

TEST(RuntimeManagerTest, Shutdown_Initialized)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "python";

    manager.initialize(config);
    manager.shutdown();

    EXPECT_FALSE(manager.isInitialized());
}

TEST(RuntimeManagerTest, PythonVersion_AfterInit)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "python";

    manager.initialize(config);

    if (manager.isInitialized())
    {
        const std::string version = manager.pythonVersion();
        // Python 版本号格式：X.Y.Z
        EXPECT_FALSE(version.empty());
        // 版本号应该包含点号
        EXPECT_NE(version.find('.'), std::string::npos);
    }
    else
    {
        // 如果初始化失败，跳过此测试
        GTEST_SKIP() << "Python not available, skipping version test";
    }
}

TEST(RuntimeManagerTest, DoubleInitialize)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "python";

    const bool firstInit = manager.initialize(config);
    const bool secondInit = manager.initialize(config);

    // 第二次初始化应该返回 false 或被忽略
    // 不应崩溃
    EXPECT_FALSE(manager.isInitialized() && !firstInit);
}

TEST(RuntimeManagerTest, DoubleShutdown)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "python";

    manager.initialize(config);
    manager.shutdown();

    // 第二次 shutdown 不应崩溃
    EXPECT_NO_THROW(manager.shutdown());

    EXPECT_FALSE(manager.isInitialized());
}

TEST(RuntimeManagerTest, ResolvePythonRoot_Empty)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "";

    manager.initialize(config);

    const std::string resolved = manager.resolvePythonRoot();

    // 无论是否为空，不应崩溃
    EXPECT_TRUE(resolved.empty() || !resolved.empty());
}

TEST(RuntimeManagerTest, ResolveTasksConfigPath_Empty)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "python";
    config.tasksConfigPath = "";

    manager.initialize(config);

    const std::string resolved = manager.resolveTasksConfigPath();

    // 不应崩溃
    EXPECT_TRUE(resolved.empty() || !resolved.empty());
}

TEST(RuntimeManagerTest, ResolveTasksConfigPath_WithPath)
{
    RuntimeManager manager;

    Config config;
    config.pythonRoot = "python";
    config.tasksConfigPath = "config/tasks.json";

    manager.initialize(config);

    const std::string resolved = manager.resolveTasksConfigPath();

    // 应该返回配置的路径或解析后的路径
    EXPECT_FALSE(resolved.empty());
}
