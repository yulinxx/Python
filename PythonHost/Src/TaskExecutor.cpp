#include "PythonHost/TaskExecutor.h"

#include "Log/SyLogger.h"
#include "Log/SyTraceContext.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>

#include <chrono>
#include <sstream>
#include <thread>

namespace PyHost
{
    namespace
    {
        std::string makeRequestId(TaskHandle handle)
        {
            const auto now = std::chrono::steady_clock::now().time_since_epoch().count();
            std::ostringstream oss;
            oss << "task-" << handle << "-" << now;
            return oss.str();
        }

        bool isCancelled(const std::unordered_map<TaskHandle, std::atomic<bool>>& flags, TaskHandle handle)
        {
            const auto it = flags.find(handle);
            return it != flags.end() && it->second.load();
        }

        std::string resolvedTraceId(const TaskRequest& request)
        {
            return SyTrace::resolveTraceIdString(request.traceId.c_str());
        }
    }  // namespace

    TaskExecutor::TaskExecutor(RuntimeManager& runtime)
        : m_runtime(runtime)
    {
    }

    TaskHandle TaskExecutor::run(const TaskRequest& request, const TaskDefinition& definition, TaskCallback callback)
    {
        const TaskHandle handle = m_nextHandle.fetch_add(1);
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_cancelFlags.emplace(handle, false);
            m_runningFlags.emplace(handle, true);
        }

        std::thread([this, request, definition, callback, handle]() {
            TaskResult result;
            result.handle = handle;
            result.taskId = request.taskId;
            result.traceId = resolvedTraceId(request);

            if (isCancelled(m_cancelFlags, handle))
            {
                result.ok = false;
                result.error = TaskError::Cancelled;
                result.message = "Task cancelled before start";
            }
            else if (definition.mode == TaskMode::Bridge)
            {
                result.ok = false;
                result.error = TaskError::ScriptFailed;
                result.message = "Bridge mode is not enabled in this build";
            }
            else
            {
                result = executeSubprocess(request, definition, handle);
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_runningFlags[handle].store(false);
                m_cancelFlags.erase(handle);
                m_runningFlags.erase(handle);
            }

            if (callback)
            {
                callback(result);
            }
        }).detach();

        return handle;
    }

    bool TaskExecutor::cancel(TaskHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_cancelFlags.find(handle);
        if (it == m_cancelFlags.end())
        {
            return false;
        }
        it->second.store(true);
        return true;
    }

    bool TaskExecutor::isRunning(TaskHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_runningFlags.find(handle);
        return it != m_runningFlags.end() && it->second.load();
    }

    TaskResult TaskExecutor::executeSubprocess(
        const TaskRequest& request, const TaskDefinition& definition, TaskHandle handle)
    {
        TaskResult result;
        result.handle = handle;
        result.taskId = request.taskId;
        result.traceId = resolvedTraceId(request);

        const char* trace = result.traceId.empty() ? "-" : result.traceId.c_str();

        if (!m_runtime.isInitialized())
        {
            result.error = TaskError::NotInitialized;
            result.message = "PythonHost runtime is not initialized";
            SY_WARNF("[PyHost] task '%s' runtime not ready trace=%s", request.taskId.c_str(), trace);
            return result;
        }

        if (isCancelled(m_cancelFlags, handle))
        {
            result.error = TaskError::Cancelled;
            result.message = "Task cancelled";
            SY_WARNF("[PyHost] task '%s' cancelled before subprocess trace=%s", request.taskId.c_str(), trace);
            return result;
        }

        const int timeoutMs = request.timeoutMs > 0 ? request.timeoutMs : definition.timeoutMs;
        const std::string pythonRoot = m_runtime.resolvePythonRoot();
        const std::string requestId = makeRequestId(handle);

        QJsonObject payload;
        payload.insert("protocol", 1);
        payload.insert("task", QString::fromStdString(request.taskId));
        payload.insert("entry", QString::fromStdString(definition.entry));
        payload.insert("request_id", QString::fromStdString(requestId));
        if (!result.traceId.empty())
        {
            payload.insert("trace_id", QString::fromStdString(result.traceId));
        }

        QJsonParseError inputParseError;
        const QJsonDocument inputDoc =
            QJsonDocument::fromJson(QByteArray::fromStdString(request.inputJson), &inputParseError);
        if (inputParseError.error != QJsonParseError::NoError || !inputDoc.isObject())
        {
            result.error = TaskError::InvalidInput;
            result.message = "Task input is not a valid JSON object";
            SY_WARNF("[PyHost] task '%s' invalid input trace=%s", request.taskId.c_str(), trace);
            return result;
        }
        payload.insert("input", inputDoc.object());

        QProcess process;
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PYTHONUTF8", "1");
        env.insert("PYTHONIOENCODING", "utf-8");
        env.insert("SANYI_PYTHON_ROOT", QString::fromStdString(pythonRoot));
        env.insert("PYTHONPATH", QString::fromStdString(pythonRoot));
        if (!result.traceId.empty())
        {
            env.insert("SANYI_TRACE_ID", QString::fromStdString(result.traceId));
        }
        process.setProcessEnvironment(env);
        process.setWorkingDirectory(QString::fromStdString(pythonRoot));
        process.setProgram(QString::fromStdString(m_runtime.pythonExecutable()));
        process.setArguments(QStringList() << "-m"
                                           << "sanyi.runtime"
                                           << "--root" << QString::fromStdString(pythonRoot));

        SY_INFOF("[PyHost] starting task '%s' handle=%llu trace=%s request_id=%s",
            request.taskId.c_str(),
            static_cast<unsigned long long>(handle),
            trace,
            requestId.c_str());

        process.start();
        if (!process.waitForStarted(5000))
        {
            result.error = TaskError::PythonNotFound;
            result.message = process.errorString().toStdString();
            SY_WARNF("[PyHost] task '%s' failed to start python trace=%s err=%s",
                request.taskId.c_str(),
                trace,
                result.message.c_str());
            return result;
        }

        const QByteArray stdinPayload = QJsonDocument(payload).toJson(QJsonDocument::Compact);
        process.write(stdinPayload);
        process.closeWriteChannel();

        if (!process.waitForFinished(timeoutMs))
        {
            process.kill();
            process.waitForFinished(3000);
            result.error = TaskError::Timeout;
            result.message = "Python task timed out";
            SY_WARNF("[PyHost] task '%s' timed out trace=%s timeout=%dms", request.taskId.c_str(), trace, timeoutMs);
            return result;
        }

        if (isCancelled(m_cancelFlags, handle))
        {
            result.error = TaskError::Cancelled;
            result.message = "Task cancelled";
            SY_WARNF("[PyHost] task '%s' cancelled during run trace=%s", request.taskId.c_str(), trace);
            return result;
        }

        const QByteArray stdoutBytes = process.readAllStandardOutput();
        const QByteArray stderrBytes = process.readAllStandardError();
        if (!stderrBytes.isEmpty())
        {
            SY_WARNF("[PyHost] task '%s' stderr trace=%s: %s", request.taskId.c_str(), trace, stderrBytes.constData());
        }

        if (process.exitCode() != 0)
        {
            result.error = TaskError::ScriptFailed;
            result.message =
                stderrBytes.isEmpty() ? "Python worker exited with non-zero status" : stderrBytes.toStdString();
            SY_WARNF("[PyHost] task '%s' exit=%d trace=%s", request.taskId.c_str(), process.exitCode(), trace);
            return result;
        }

        QJsonParseError parseError;
        const QJsonDocument responseDoc = QJsonDocument::fromJson(stdoutBytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !responseDoc.isObject())
        {
            result.error = TaskError::ProtocolError;
            result.message = "Invalid JSON response from Python worker";
            SY_WARNF("[PyHost] task '%s' invalid response trace=%s", request.taskId.c_str(), trace);
            return result;
        }

        const QJsonObject responseObj = responseDoc.object();
        result.ok = responseObj.value("ok").toBool(false);
        if (responseObj.value("error").isString())
        {
            result.message = responseObj.value("error").toString().toStdString();
        }

        if (responseObj.contains("result"))
        {
            const QJsonValue resultValue = responseObj.value("result");
            result.resultJson = QJsonDocument(
                resultValue.isObject() ? QJsonObject(resultValue.toObject()) : QJsonObject{ { "value", resultValue } })
                                    .toJson(QJsonDocument::Compact)
                                    .toStdString();
        }
        if (responseObj.contains("metrics") && responseObj.value("metrics").isObject())
        {
            result.metricsJson =
                QJsonDocument(responseObj.value("metrics").toObject()).toJson(QJsonDocument::Compact).toStdString();
        }

        if (!result.ok && result.error == TaskError::None)
        {
            result.error = TaskError::ScriptFailed;
        }

        SY_INFOF("[PyHost] task '%s' finished ok=%d trace=%s handle=%llu",
            request.taskId.c_str(),
            result.ok ? 1 : 0,
            trace,
            static_cast<unsigned long long>(handle));

        return result;
    }
}  // namespace PyHost