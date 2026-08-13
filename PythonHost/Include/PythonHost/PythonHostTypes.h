#pragma once

#include "PythonHost/PythonHostAPI.h"

#include <cstdint>
#include <functional>
#include <string>

namespace PyHost
{
    using TaskHandle = uint64_t;
    using JsonString = std::string;

    enum class TaskError
    {
        None = 0,
        PythonNotFound,
        ScriptFailed,
        Timeout,
        Cancelled,
        InvalidInput,
        ProtocolError,
        UnknownTask,
        NotInitialized,
    };

    enum class TaskMode
    {
        Subprocess,
        Bridge,
    };

    enum class TaskPriority
    {
        Low,
        Normal,
        High,
    };

    struct TaskRequest
    {
        std::string taskId;
        JsonString inputJson = "{}";
        /** 可选；为空时继承 SyTrace::currentTraceIdString() */
        std::string traceId;
        int timeoutMs = 60000;
        bool cancellable = true;
        TaskPriority priority = TaskPriority::Normal;
    };

    struct TaskResult
    {
        bool ok = false;
        TaskError error = TaskError::None;
        std::string message;
        JsonString resultJson = "{}";
        JsonString metricsJson = "{}";
        TaskHandle handle = 0;
        std::string taskId;
        std::string traceId;
    };

    struct TaskDefinition
    {
        std::string taskId;
        std::string entry;
        TaskMode mode = TaskMode::Subprocess;
        int timeoutMs = 60000;
    };

    struct Config
    {
        std::string pythonRoot;
        std::string venvPath;
        std::string extensionPath;
        std::string tasksConfigPath;
        std::string sourcePythonRoot;
        bool enableEmbedded = false;
        bool dispatchCallbacksOnMainThread = true;
        int workerThreads = 2;
    };

    struct RuntimeInfo
    {
        bool initialized = false;
        std::string pythonExecutable;
        std::string pythonRoot;
        std::string pythonVersion;
        int registeredTaskCount = 0;
    };

    using TaskCallback = std::function<void(const TaskResult&)>;
}  // namespace PyHost
