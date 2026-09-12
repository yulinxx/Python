/**
 * @file TaskRegistryTests.cpp
 * @brief TaskRegistry 单元测试
 *
 * 覆盖:
 * - load / clear 任务配置加载
 * - lookup 任务查找
 * - taskCount 任务计数
 */
#include <gtest/gtest.h>

#include "PythonHost/TaskRegistry.h"
#include <fstream>
#include <filesystem>

namespace
{
    // 创建临时任务配置文件
    std::string createTempTasksConfig(const std::string& content)
    {
        auto tempPath = std::filesystem::temp_directory_path() / "test_tasks.json";
        std::ofstream ofs(tempPath);
        ofs << content;
        ofs.close();
        return tempPath.string();
    }

    void cleanupTempFile(const std::string& path)
    {
        std::filesystem::remove(path);
    }
}  // namespace

TEST(TaskRegistryTest, Load_ValidConfig)
{
    PyHost::TaskRegistry registry;

    const std::string config = R"({
        "tasks": [
            {
                "taskId": "test_task_1",
                "entry": "scripts/test.py",
                "mode": "Subprocess",
                "timeoutMs": 30000
            }
        ]
    })";

    const std::string configPath = createTempTasksConfig(config);
    const bool result = registry.load(configPath);

    EXPECT_TRUE(result);
    EXPECT_EQ(registry.taskCount(), 1);

    cleanupTempFile(configPath);
}

TEST(TaskRegistryTest, Load_InvalidPath)
{
    PyHost::TaskRegistry registry;

    const bool result = registry.load("invalid_path/not_exist.json");

    EXPECT_FALSE(result);
    EXPECT_EQ(registry.taskCount(), 0);
}

TEST(TaskRegistryTest, Load_InvalidJson)
{
    PyHost::TaskRegistry registry;

    const std::string config = "this is not valid json";
    const std::string configPath = createTempTasksConfig(config);
    const bool result = registry.load(configPath);

    EXPECT_FALSE(result);

    cleanupTempFile(configPath);
}

TEST(TaskRegistryTest, Lookup_ExistingTask)
{
    PyHost::TaskRegistry registry;

    const std::string config = R"({
        "tasks": [
            {
                "taskId": "my_task",
                "entry": "scripts/my_task.py",
                "mode": "Subprocess",
                "timeoutMs": 5000
            }
        ]
    })";

    const std::string configPath = createTempTasksConfig(config);
    registry.load(configPath);

    PyHost::TaskDefinition definition;
    const bool found = registry.lookup("my_task", definition);

    EXPECT_TRUE(found);
    EXPECT_EQ(definition.taskId, "my_task");
    EXPECT_EQ(definition.entry, "scripts/my_task.py");
    EXPECT_EQ(definition.timeoutMs, 5000);

    cleanupTempFile(configPath);
}

TEST(TaskRegistryTest, Lookup_NonExistingTask)
{
    PyHost::TaskRegistry registry;

    const std::string config = R"({
        "tasks": [
            {
                "taskId": "existing_task",
                "entry": "scripts/test.py",
                "mode": "Subprocess"
            }
        ]
    })";

    const std::string configPath = createTempTasksConfig(config);
    registry.load(configPath);

    PyHost::TaskDefinition definition;
    const bool found = registry.lookup("non_existing_task", definition);

    EXPECT_FALSE(found);

    cleanupTempFile(configPath);
}

TEST(TaskRegistryTest, Clear_AfterLoad)
{
    PyHost::TaskRegistry registry;

    const std::string config = R"({
        "tasks": [
            {
                "taskId": "task1",
                "entry": "scripts/test.py"
            },
            {
                "taskId": "task2",
                "entry": "scripts/test2.py"
            }
        ]
    })";

    const std::string configPath = createTempTasksConfig(config);
    registry.load(configPath);

    EXPECT_EQ(registry.taskCount(), 2);

    registry.clear();

    EXPECT_EQ(registry.taskCount(), 0);

    cleanupTempFile(configPath);
}

TEST(TaskRegistryTest, Load_MultipleTasks)
{
    PyHost::TaskRegistry registry;

    const std::string config = R"({
        "tasks": [
            {
                "taskId": "task_a",
                "entry": "scripts/a.py",
                "timeoutMs": 10000
            },
            {
                "taskId": "task_b",
                "entry": "scripts/b.py",
                "timeoutMs": 20000
            },
            {
                "taskId": "task_c",
                "entry": "scripts/c.py",
                "timeoutMs": 30000
            }
        ]
    })";

    const std::string configPath = createTempTasksConfig(config);
    const bool result = registry.load(configPath);

    EXPECT_TRUE(result);
    EXPECT_EQ(registry.taskCount(), 3);

    // 验证每个任务都能被找到
    PyHost::TaskDefinition def;
    EXPECT_TRUE(registry.lookup("task_a", def));
    EXPECT_TRUE(registry.lookup("task_b", def));
    EXPECT_TRUE(registry.lookup("task_c", def));

    cleanupTempFile(configPath);
}

TEST(TaskRegistryTest, TaskCount_EmptyRegistry)
{
    PyHost::TaskRegistry registry;

    EXPECT_EQ(registry.taskCount(), 0);
}

TEST(TaskRegistryTest, DefaultTaskMode)
{
    PyHost::TaskRegistry registry;

    const std::string config = R"({
        "tasks": [
            {
                "taskId": "default_mode_task",
                "entry": "scripts/test.py"
            }
        ]
    })";

    const std::string configPath = createTempTasksConfig(config);
    registry.load(configPath);

    PyHost::TaskDefinition definition;
    registry.lookup("default_mode_task", definition);

    // 默认模式应为 Subprocess
    EXPECT_EQ(definition.mode, PyHost::TaskMode::Subprocess);
    // 默认超时应为 60000ms
    EXPECT_EQ(definition.timeoutMs, 60000);

    cleanupTempFile(configPath);
}
