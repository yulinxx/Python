#pragma once

#include "PythonHost/PythonHostTypes.h"
#include "PythonHost/RuntimeManager.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <thread>
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
        TaskResult executeSubprocess(const TaskRequest& request, const TaskDefinition& definition, TaskHandle handle);

        /// 析构清理：置所有 cancel flag → kill 所有 QProcess → join 所有线程（带超时）
        void shutdownAll();

        RuntimeManager& m_runtime;
        std::atomic<TaskHandle> m_nextHandle{ 1 };
        mutable std::mutex m_mutex;
        std::unordered_map<TaskHandle, std::atomic<bool>> m_cancelFlags;
        std::unordered_map<TaskHandle, std::atomic<bool>> m_runningFlags;
        /// 活跃 worker 线程：handle → std::thread，析构时统一 join
        std::unordered_map<TaskHandle, std::thread> m_threads;
        /// QProcess 指针（worker 持有 unique_ptr，析构清理时需要先 kill 再 join）
        std::unordered_map<TaskHandle, QProcess*> m_processes;
    };
}  // namespace PyHost
