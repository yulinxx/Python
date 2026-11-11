#pragma once

#include "PythonHost/PythonHostTypes.h"
#include "PythonHost/RuntimeManager.h"

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <unordered_map>

class QProcess;

namespace PyHost
{
    class TaskExecutor
    {
    public:
        explicit TaskExecutor(RuntimeManager& runtime);
        ~TaskExecutor();

        TaskHandle run(const TaskRequest& request, const TaskDefinition& definition, TaskCallback callback);

        bool cancel(TaskHandle handle);
        bool isRunning(TaskHandle handle) const;

    private:
        TaskResult executeSubprocess(const TaskRequest& request, const TaskDefinition& definition, TaskHandle handle,
            const std::shared_ptr<std::atomic<bool>>& cancelFlag);

        /// 析构清理：置所有 cancel flag → kill 所有 QProcess → 等待所有 worker 退出
        void shutdownAll();

        RuntimeManager& m_runtime;
        std::atomic<TaskHandle> m_nextHandle{ 1 };
        mutable std::mutex m_mutex;
        /// 取消标志逐任务独占：worker 持 shared_ptr 副本，读取消状态不再触碰共享容器
        std::unordered_map<TaskHandle, std::shared_ptr<std::atomic<bool>>> m_cancelFlags;
        /// 运行中的任务（只在 m_mutex 保护下读写）
        std::unordered_map<TaskHandle, bool> m_runningFlags;
        /// QProcess 指针（worker 在栈上持有，析构清理时需要先 kill）
        std::unordered_map<TaskHandle, QProcess*> m_processes;
        /// 活跃 worker 计数与退出通知：线程 detach 后不保存 std::thread 对象，
        /// 销毁时按计数等待，既不会析构 joinable 线程，也不会让 worker 越过 this 生命周期
        std::condition_variable m_workerExitCv;
        size_t m_activeWorkers = 0;
        /// 关停标志：置位后拒绝新任务，并丢弃尚未投递的完成回调
        bool m_shuttingDown = false;
    };
}  // namespace PyHost
