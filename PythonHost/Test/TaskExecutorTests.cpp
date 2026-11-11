/**
 * @file TaskExecutorTests.cpp
 * @brief TaskExecutor 线程生命周期与取消标志测试
 *
 * 覆盖:
 * - 任务正常结束路径（工作线程退出时不得析构 joinable 的 std::thread）
 * - 多任务并发结束后的状态清理
 * - cancel / isRunning 对外语义
 *
 * 说明：全部用例走 Bridge 模式，不启动 Python 子进程，
 *       因此不依赖宿主环境是否安装了 Python。
 */
#include <gtest/gtest.h>

#include "PythonHost/TaskExecutor.h"

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

using namespace PyHost;

namespace
{
    // 轮询等待条件成立，超时返回 false
    bool waitUntil(const std::function<bool()>& predicate, std::chrono::milliseconds timeout)
    {
        const auto deadline = std::chrono::steady_clock::now() + timeout;
        while (std::chrono::steady_clock::now() < deadline)
        {
            if (predicate())
            {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        return predicate();
    }

    TaskDefinition makeBridgeDefinition()
    {
        TaskDefinition definition;
        definition.taskId = "test.bridge";
        definition.entry = "test.entry";
        definition.mode = TaskMode::Bridge;
        definition.timeoutMs = 1000;
        return definition;
    }

    TaskRequest makeRequest()
    {
        TaskRequest request;
        request.taskId = "test.bridge";
        request.timeoutMs = 1000;
        return request;
    }
}  // namespace

// 工作线程跑完整个任务后进程仍存活：修复前 worker 会在自己仍运行时
// 擦除 m_threads 中的条目，析构 joinable 的 std::thread 触发 std::terminate
TEST(TaskExecutorTest, WorkerCompletionDoesNotTerminate)
{
    RuntimeManager runtime;
    TaskExecutor executor(runtime);

    std::mutex mutex;
    std::condition_variable cv;
    bool delivered = false;
    TaskResult captured;

    const TaskHandle handle = executor.run(makeRequest(), makeBridgeDefinition(),
        [&](const TaskResult& result) {
            std::lock_guard<std::mutex> lock(mutex);
            captured = result;
            delivered = true;
            cv.notify_all();
        });

    ASSERT_NE(handle, 0u);

    std::unique_lock<std::mutex> lock(mutex);
    const bool finished = cv.wait_for(lock, std::chrono::seconds(5), [&]() { return delivered; });
    EXPECT_TRUE(finished);
    EXPECT_EQ(captured.handle, handle);
    EXPECT_FALSE(captured.ok);
    EXPECT_EQ(captured.error, TaskError::ScriptFailed);

    // 作用域结束时析构 executor，验证销毁路径不阻塞、不崩溃
}

// 多个任务并发结束后不得残留线程对象，也不得重复投递回调
TEST(TaskExecutorTest, ConcurrentTasksCompleteOnce)
{
    RuntimeManager runtime;
    TaskExecutor executor(runtime);

    constexpr int kTaskCount = 16;
    std::atomic<int> delivered{ 0 };

    for (int i = 0; i < kTaskCount; ++i)
    {
        executor.run(makeRequest(), makeBridgeDefinition(), [&](const TaskResult&) {
            delivered.fetch_add(1);
        });
    }

    const bool allDone = waitUntil([&]() { return delivered.load() == kTaskCount; }, std::chrono::seconds(10));
    EXPECT_TRUE(allDone);
    EXPECT_EQ(delivered.load(), kTaskCount);

    // 让最后一批 worker 完成收尾，再走销毁路径
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
}

// 未知 handle 的取消请求必须被拒绝，不能凭空建出取消标志
TEST(TaskExecutorTest, CancelUnknownHandleReturnsFalse)
{
    RuntimeManager runtime;
    TaskExecutor executor(runtime);

    EXPECT_FALSE(executor.cancel(0));
    EXPECT_FALSE(executor.cancel(123456));
    EXPECT_FALSE(executor.isRunning(123456));
}

// 任务结束后 isRunning 必须回到 false
TEST(TaskExecutorTest, IsRunningFalseAfterCompletion)
{
    RuntimeManager runtime;
    TaskExecutor executor(runtime);

    std::atomic<bool> delivered{ false };
    const TaskHandle handle = executor.run(makeRequest(), makeBridgeDefinition(),
        [&](const TaskResult&) { delivered.store(true); });
    ASSERT_NE(handle, 0u);

    ASSERT_TRUE(waitUntil([&]() { return delivered.load(); }, std::chrono::seconds(5)));
    EXPECT_FALSE(executor.isRunning(handle));

    // 已结束的任务不再接受取消
    EXPECT_FALSE(executor.cancel(handle));
}
