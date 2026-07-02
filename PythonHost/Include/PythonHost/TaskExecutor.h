#pragma once

#include "PythonHost/PythonHostTypes.h"
#include "PythonHost/RuntimeManager.h"

#include <atomic>
#include <functional>
#include <mutex>
#include <unordered_map>

namespace PyHost
{
    class TaskExecutor
    {
    public:
        explicit TaskExecutor(RuntimeManager& runtime);

        TaskHandle run(const TaskRequest& request,
            const TaskDefinition& definition,
            TaskCallback callback);

        bool cancel(TaskHandle handle);
        bool isRunning(TaskHandle handle) const;

    private:
        TaskResult executeSubprocess(const TaskRequest& request,
            const TaskDefinition& definition,
            TaskHandle handle);

        RuntimeManager& m_runtime;
        std::atomic<TaskHandle> m_nextHandle{ 1 };
        mutable std::mutex m_mutex;
        std::unordered_map<TaskHandle, std::atomic<bool>> m_cancelFlags;
        std::unordered_map<TaskHandle, std::atomic<bool>> m_runningFlags;
    };
}
