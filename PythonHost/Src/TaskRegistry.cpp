#include "PythonHost/TaskRegistry.h"

#include "Log/SyLogger.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

namespace PyHost
{
    namespace
    {
        TaskMode parseMode(const QString& mode)
        {
            if (mode == "bridge")
            {
                return TaskMode::Bridge;
            }
            return TaskMode::Subprocess;
        }
    }  // namespace

    bool TaskRegistry::load(const std::string& tasksConfigPath)
    {
        m_tasks.clear();

        QFile file(QString::fromStdString(tasksConfigPath));
        if (!file.open(QIODevice::ReadOnly))
        {
            SY_WARNF("[PyHost] tasks.json not found: %s", tasksConfigPath.c_str());
            return false;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &parseError);
        if (parseError.error != QJsonParseError::NoError || !doc.isObject())
        {
            SY_ERRORF("[PyHost] Failed to parse tasks.json: %s", parseError.errorString().toUtf8().constData());
            return false;
        }

        const QJsonObject root = doc.object();
        const QJsonObject tasks = root.value("tasks").toObject();
        for (auto it = tasks.begin(); it != tasks.end(); ++it)
        {
            if (!it.value().isObject())
            {
                continue;
            }

            const QJsonObject taskObj = it.value().toObject();
            TaskDefinition definition;
            definition.taskId = it.key().toStdString();
            definition.entry = taskObj.value("entry").toString().toStdString();
            definition.mode = parseMode(taskObj.value("mode").toString("subprocess"));
            definition.timeoutMs = taskObj.value("timeout_ms").toInt(60000);

            if (definition.entry.empty())
            {
                SY_WARNF("[PyHost] Task '%s' missing entry, skipped", definition.taskId.c_str());
                continue;
            }

            m_tasks.emplace(definition.taskId, definition);
        }

        SY_INFOF("[PyHost] Loaded %d task(s) from %s", static_cast<int>(m_tasks.size()), tasksConfigPath.c_str());
        return !m_tasks.empty();
    }

    void TaskRegistry::clear()
    {
        m_tasks.clear();
    }

    bool TaskRegistry::lookup(const std::string& taskId, TaskDefinition& outDefinition) const
    {
        const auto it = m_tasks.find(taskId);
        if (it == m_tasks.end())
        {
            return false;
        }
        outDefinition = it->second;
        return true;
    }

    int TaskRegistry::taskCount() const
    {
        return static_cast<int>(m_tasks.size());
    }
}  // namespace PyHost