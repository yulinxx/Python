#include "PythonHost/PythonHost.h"

#include "PythonHost/MainThreadDispatch.h"
#include "PythonHost/TaskExecutor.h"
#include "PythonHost/TaskRegistry.h"
#include "Log/SyLogger.h"
#include "Log/SyTraceContext.h"

namespace PyHost
{
    namespace
    {
        RuntimeManager g_runtime;
        TaskRegistry g_registry;
        TaskExecutor g_executor(g_runtime);
    }

    PythonHost& PythonHost::instance()
    {
        static PythonHost host;
        return host;
    }

    PythonHost::PythonHost() = default;
    PythonHost::~PythonHost() = default;

    bool PythonHost::initialize(const Config& config)
    {
        if (m_initialized)
            return true;

        m_config = config;

        if (!g_runtime.initialize(config))
        {
            SY_ERROR("[PyHost] RuntimeManager initialization failed");
            return false;
        }

        const std::string tasksPath = g_runtime.resolveTasksConfigPath();
        if (!g_registry.load(tasksPath))
        {
            SY_WARN("[PyHost] Task registry loaded zero tasks; check Python/tasks.json deployment");
        }

        m_initialized = true;
        SY_INFO("[PyHost] PythonHost initialized");
        return true;
    }

    void PythonHost::shutdown()
    {
        if (!m_initialized)
            return;

        g_runtime.shutdown();
        g_registry.clear();
        m_initialized = false;
        SY_INFO("[PyHost] PythonHost shutdown");
    }

    bool PythonHost::isInitialized() const
    {
        return m_initialized;
    }

    TaskHandle PythonHost::runTask(const TaskRequest& request, TaskCallback callback)
    {
        TaskRequest traced = request;
        traced.traceId = SyTrace::resolveTraceId(request.traceId);

        if (!m_initialized)
        {
            TaskResult result;
            result.taskId = traced.taskId;
            result.traceId = traced.traceId;
            result.ok = false;
            result.error = TaskError::NotInitialized;
            result.message = "PythonHost is not initialized";
            SY_WARNF("[PyHost] task '%s' rejected (not initialized) trace=%s",
                traced.taskId.c_str(),
                traced.traceId.empty() ? "-" : traced.traceId.c_str());
            deliverCallback(callback, result);
            return 0;
        }

        TaskDefinition definition;
        if (!g_registry.lookup(traced.taskId, definition))
        {
            TaskResult result;
            result.taskId = traced.taskId;
            result.traceId = traced.traceId;
            result.ok = false;
            result.error = TaskError::UnknownTask;
            result.message = "Unknown task id: " + traced.taskId;
            SY_WARNF("[PyHost] unknown task '%s' trace=%s",
                traced.taskId.c_str(),
                traced.traceId.empty() ? "-" : traced.traceId.c_str());
            deliverCallback(callback, result);
            return 0;
        }

        SY_INFOF("[PyHost] dispatch task='%s' trace=%s timeout=%dms",
            traced.taskId.c_str(),
            traced.traceId.empty() ? "-" : traced.traceId.c_str(),
            traced.timeoutMs);

        return g_executor.run(traced, definition, [this, callback](const TaskResult& result) {
            deliverCallback(callback, result);
            });
    }

    bool PythonHost::cancel(TaskHandle handle)
    {
        return g_executor.cancel(handle);
    }

    RuntimeInfo PythonHost::info() const
    {
        RuntimeInfo info;
        info.initialized = m_initialized;
        info.pythonExecutable = g_runtime.pythonExecutable();
        info.pythonRoot = g_runtime.pythonRoot();
        info.pythonVersion = g_runtime.pythonVersion();
        info.registeredTaskCount = g_registry.taskCount();
        return info;
    }

    const Config& PythonHost::config() const
    {
        return m_config;
    }

    void PythonHost::deliverCallback(TaskCallback callback, const TaskResult& result)
    {
        if (!callback)
            return;

        if (m_config.dispatchCallbacksOnMainThread)
        {
            postToMainThread([callback, result]() {
                callback(result);
                });
            return;
        }

        callback(result);
    }
}