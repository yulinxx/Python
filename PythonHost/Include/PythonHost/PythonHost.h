#pragma once

#include "PythonHost/PythonHostAPI.h"
#include "PythonHost/PythonHostTypes.h"

namespace PyHost
{
    class PYTHONHOST_API PythonHost
    {
    public:
        static PythonHost& instance();
        PythonHost(const PythonHost&) = delete;
        PythonHost& operator=(const PythonHost&) = delete;

    public:
        bool initialize(const Config& config);
        void shutdown();
        bool isInitialized() const;

        TaskHandle runTask(const TaskRequest& request, TaskCallback callback);
        bool cancel(TaskHandle handle);

        RuntimeInfo info() const;

        const Config& config() const;

    private:
        PythonHost();
        ~PythonHost();

    private:
        void deliverCallback(TaskCallback callback, const TaskResult& result);

        Config m_config;
        bool m_initialized = false;
    };
}  // namespace PyHost
