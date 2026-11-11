#include "PythonHost/TaskExecutor.h"

#include "Log/AprLogger.h"
#include "Log/AprTraceContext.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QProcessEnvironment>

#include <chrono>
#include <memory>
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

        bool isCancelled(const std::shared_ptr<std::atomic<bool>>& flag)
        {
            return flag && flag->load();
        }

        std::string resolvedTraceId(const TaskRequest& request)
        {
            return AprTrace::resolveTraceIdString(request.traceId.c_str());
        }
    }  // namespace

    TaskExecutor::TaskExecutor(RuntimeManager& runtime)
        : m_runtime(runtime)
    {
    }

    TaskExecutor::~TaskExecutor()
    {
        shutdownAll();
    }

    void TaskExecutor::shutdownAll()
    {
        // 三步清理：置 cancel flag → kill 所有 QProcess → 等待 worker 退出
        // 顺序不能反：先 kill 让阻塞在 waitForFinished 的 worker 立即返回，
        // 之后按计数等待才能在可见时间内收敛
        size_t killedCount = 0;
        size_t pendingWorkers = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_shuttingDown = true;
            // 置所有取消标志
            for (auto& [h, flag] : m_cancelFlags)
            {
                if (flag)
                {
                    flag->store(true);
                }
            }
            // kill 必须在锁内：worker 只有在 ProcessGuard 析构时拿到同一把锁才会擦除登记，
            // 因此这里 kill 的 QProcess 对象在 kill 期间保证仍然存活
            for (auto& [h, proc] : m_processes)
            {
                if (proc)
                {
                    proc->kill();
                    ++killedCount;
                }
            }
            pendingWorkers = m_activeWorkers;
        }
        APR_INFOF("[PyHost] shutdownAll: %zu process(es) killed, waiting for %zu worker(s)",
            killedCount, pendingWorkers);

        // worker 在线程函数末尾递减计数并 notify；计数归零前本对象与 m_runtime 都不得析构。
        // 这里等待而不是 join：线程已 detach，不存在可 join 的线程对象
        std::unique_lock<std::mutex> lock(m_mutex);
        while (m_activeWorkers > 0)
        {
            if (m_workerExitCv.wait_for(lock, std::chrono::seconds(3)) == std::cv_status::timeout)
            {
                APR_WARNF("[PyHost] shutdownAll: %zu worker(s) still running, keep waiting", m_activeWorkers);
            }
        }

        // 最终清理残留的 map 条目
        m_cancelFlags.clear();
        m_runningFlags.clear();
        m_processes.clear();
        lock.unlock();
        APR_INFO("[PyHost] shutdownAll: all workers exited and state cleared");
    }

    TaskHandle TaskExecutor::run(const TaskRequest& request, const TaskDefinition& definition, TaskCallback callback)
    {
        // 取消标志逐任务新建，worker 拿到的是独享副本，读取时不访问任何共享容器
        auto cancelFlag = std::make_shared<std::atomic<bool>>(false);
        TaskHandle handle = 0;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_shuttingDown)
            {
                // 关停后不得再起线程：worker 已 detach，晚于析构创建会访问已销毁的 this
                APR_WARNF("[PyHost] task '%s' rejected (executor is shutting down)", request.taskId.c_str());
            }
            else
            {
                handle = m_nextHandle.fetch_add(1);
                m_cancelFlags.emplace(handle, cancelFlag);
                m_runningFlags.emplace(handle, true);
                ++m_activeWorkers;
            }
        }

        if (handle == 0)
        {
            TaskResult rejected;
            rejected.taskId = request.taskId;
            rejected.traceId = resolvedTraceId(request);
            rejected.ok = false;
            rejected.error = TaskError::Cancelled;
            rejected.message = "Task executor is shutting down";
            if (callback)
            {
                callback(rejected);
            }
            return 0;
        }

        // 工作线程 detach 后不再被任何容器持有，因此不存在“工作线程运行中析构自己的 std::thread”这条路径；
        // 生命周期由 shutdownAll() 按 m_activeWorkers 计数统一等待
        std::thread([this, request, definition, callback, handle, cancelFlag]() {
            TaskResult result;
            result.handle = handle;
            result.taskId = request.taskId;
            result.traceId = resolvedTraceId(request);

            if (isCancelled(cancelFlag))
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
                result = executeSubprocess(request, definition, handle, cancelFlag);
            }

            bool deliver = false;
            {
                std::lock_guard<std::mutex> lock(m_mutex);
                // 关停中丢弃回调：宿主对象可能已经在析构，不能再被回调触达
                deliver = !m_shuttingDown;
            }

            // 回调先于计数递减执行：shutdownAll() 等待计数归零，回调在栈上必然已经返回
            if (callback && deliver)
            {
                callback(result);
            }

            {
                std::lock_guard<std::mutex> lock(m_mutex);
                m_runningFlags.erase(handle);
                m_cancelFlags.erase(handle);
                if (m_activeWorkers > 0)
                {
                    --m_activeWorkers;
                }
            }
            m_workerExitCv.notify_all();
        }).detach();

        return handle;
    }

    bool TaskExecutor::cancel(TaskHandle handle)
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_cancelFlags.find(handle);
        if (it == m_cancelFlags.end() || !it->second)
        {
            return false;
        }
        it->second->store(true);
        return true;
    }

    bool TaskExecutor::isRunning(TaskHandle handle) const
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        const auto it = m_runningFlags.find(handle);
        return it != m_runningFlags.end() && it->second;
    }

    TaskResult TaskExecutor::executeSubprocess(const TaskRequest& request, const TaskDefinition& definition,
        TaskHandle handle, const std::shared_ptr<std::atomic<bool>>& cancelFlag)
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
            APR_WARNF("[PyHost] task '%s' runtime not ready trace=%s", request.taskId.c_str(), trace);
            return result;
        }

        if (isCancelled(cancelFlag))
        {
            result.error = TaskError::Cancelled;
            result.message = "Task cancelled";
            APR_WARNF("[PyHost] task '%s' cancelled before subprocess trace=%s", request.taskId.c_str(), trace);
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
            APR_WARNF("[PyHost] task '%s' invalid input trace=%s", request.taskId.c_str(), trace);
            return result;
        }
        payload.insert("input", inputDoc.object());

        QProcess process;
        // 注册到 m_processes 供 shutdownAll kill；RAII guard 确保所有 return 路径自动擦除
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            m_processes[handle] = &process;
        }
        struct ProcessGuard { TaskExecutor* o; TaskHandle h; ProcessGuard(TaskExecutor* oo, TaskHandle hh) : o(oo), h(hh) {}
            ~ProcessGuard() { std::lock_guard<std::mutex> lk(o->m_mutex); o->m_processes.erase(h); }
        } procGuard(this, handle);

        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("PYTHONUTF8", "1");
        env.insert("PYTHONIOENCODING", "utf-8");
        env.insert("APR_PYTHON_ROOT", QString::fromStdString(pythonRoot));
        env.insert("PYTHONPATH", QString::fromStdString(pythonRoot));
        if (!result.traceId.empty())
        {
            env.insert("APR_TRACE_ID", QString::fromStdString(result.traceId));
        }
        process.setProcessEnvironment(env);
        process.setWorkingDirectory(QString::fromStdString(pythonRoot));
        process.setProgram(QString::fromStdString(m_runtime.pythonExecutable()));
        process.setArguments(QStringList() << "-m"
                                           << "apr.runtime"
                                           << "--root" << QString::fromStdString(pythonRoot));

        APR_INFOF("[PyHost] starting task '%s' handle=%llu trace=%s request_id=%s",
            request.taskId.c_str(),
            static_cast<unsigned long long>(handle),
            trace,
            requestId.c_str());

        process.start();
        if (!process.waitForStarted(5000))
        {
            result.error = TaskError::PythonNotFound;
            result.message = process.errorString().toStdString();
            APR_WARNF("[PyHost] task '%s' failed to start python trace=%s err=%s",
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
            APR_WARNF("[PyHost] task '%s' timed out trace=%s timeout=%dms", request.taskId.c_str(), trace, timeoutMs);
            return result;
        }

        if (isCancelled(cancelFlag))
        {
            result.error = TaskError::Cancelled;
            result.message = "Task cancelled";
            APR_WARNF("[PyHost] task '%s' cancelled during run trace=%s", request.taskId.c_str(), trace);
            return result;
        }

        const QByteArray stdoutBytes = process.readAllStandardOutput();
        const QByteArray stderrBytes = process.readAllStandardError();
        if (!stderrBytes.isEmpty())
        {
            APR_WARNF("[PyHost] task '%s' stderr trace=%s: %s", request.taskId.c_str(), trace, stderrBytes.constData());
        }

        if (process.exitCode() != 0)
        {
            result.error = TaskError::ScriptFailed;
            result.message =
                stderrBytes.isEmpty() ? "Python worker exited with non-zero status" : stderrBytes.toStdString();
            APR_WARNF("[PyHost] task '%s' exit=%d trace=%s", request.taskId.c_str(), process.exitCode(), trace);
            return result;
        }

        QJsonParseError parseError;
        const QJsonDocument responseDoc = QJsonDocument::fromJson(stdoutBytes, &parseError);
        if (parseError.error != QJsonParseError::NoError || !responseDoc.isObject())
        {
            result.error = TaskError::ProtocolError;
            result.message = "Invalid JSON response from Python worker";
            APR_WARNF("[PyHost] task '%s' invalid response trace=%s", request.taskId.c_str(), trace);
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

        APR_INFOF("[PyHost] task '%s' finished ok=%d trace=%s handle=%llu",
            request.taskId.c_str(),
            result.ok ? 1 : 0,
            trace,
            static_cast<unsigned long long>(handle));

        return result;
    }
}  // namespace PyHost