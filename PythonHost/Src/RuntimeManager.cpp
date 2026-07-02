#include "PythonHost/RuntimeManager.h"

#include "Log/SyLogger.h"

#include <QProcess>
#include <QFileInfo>
#include <QDir>

#include <cstdlib>
#include <filesystem>

namespace fs = std::filesystem;

namespace PyHost
{
    namespace
    {
        std::string envOrEmpty(const char* name)
        {
            const char* value = std::getenv(name);
            return value ? std::string(value) : std::string();
        }

        std::string venvPythonPath(const std::string& venvPath)
        {
#ifdef _WIN32
            return (fs::path(venvPath) / "Scripts" / "python.exe").string();
#else
            return (fs::path(venvPath) / "bin" / "python3").string();
#endif
        }
    }

    bool RuntimeManager::initialize(const Config& config)
    {
        m_config = config;
        m_pythonExecutable.clear();
        m_pythonVersion.clear();

        const std::string bundledPython = venvPythonPath(m_config.venvPath);
        if (fs::exists(bundledPython) && probePythonExecutable(bundledPython))
        {
            m_initialized = true;
            SY_INFOF("[PyHost] Using bundled venv Python: %s", m_pythonExecutable.c_str());
            return true;
        }

        const std::string customHome = envOrEmpty("SANYI_PYTHON_HOME");
        if (!customHome.empty())
        {
#ifdef _WIN32
            const std::string candidate = (fs::path(customHome) / "python.exe").string();
#else
            const std::string candidate = (fs::path(customHome) / "bin" / "python3").string();
#endif
            if (probePythonExecutable(candidate))
            {
                m_initialized = true;
                SY_INFOF("[PyHost] Using SANYI_PYTHON_HOME Python: %s", m_pythonExecutable.c_str());
                return true;
            }
        }

        QStringList candidates;
#ifdef Q_OS_WIN
        candidates << "python" << "python3" << "py";
#else
        candidates << "python3" << "python";
#endif

        for (const QString& candidate : candidates)
        {
            if (probePythonExecutable(candidate.toStdString()))
            {
                m_initialized = true;
                SY_WARNF("[PyHost] Falling back to system Python: %s", m_pythonExecutable.c_str());
                return true;
            }
        }

        SY_ERROR("[PyHost] Python executable not found");
        m_initialized = false;
        return false;
    }

    void RuntimeManager::shutdown()
    {
        m_initialized = false;
        m_pythonExecutable.clear();
        m_pythonVersion.clear();
    }

    bool RuntimeManager::isInitialized() const
    {
        return m_initialized;
    }

    std::string RuntimeManager::pythonExecutable() const
    {
        return m_pythonExecutable;
    }

    std::string RuntimeManager::pythonRoot() const
    {
        return resolvePythonRoot();
    }

    std::string RuntimeManager::pythonVersion() const
    {
        return m_pythonVersion;
    }

    std::string RuntimeManager::resolvePythonRoot() const
    {
        if (fs::exists(m_config.pythonRoot))
            return m_config.pythonRoot;
        if (!m_config.sourcePythonRoot.empty() && fs::exists(m_config.sourcePythonRoot))
            return m_config.sourcePythonRoot;
        return m_config.pythonRoot;
    }

    std::string RuntimeManager::resolveTasksConfigPath() const
    {
        if (fs::exists(m_config.tasksConfigPath))
            return m_config.tasksConfigPath;

        if (!m_config.sourcePythonRoot.empty())
        {
            const std::string devTasks = (fs::path(m_config.sourcePythonRoot) / "tasks.json").string();
            if (fs::exists(devTasks))
                return devTasks;
        }

        return m_config.tasksConfigPath;
    }

    bool RuntimeManager::probePythonExecutable(const std::string& candidate)
    {
        if (candidate.empty())
            return false;

        QProcess process;
        process.setProgram(QString::fromStdString(candidate));
        process.setArguments(QStringList() << "--version");
        process.start();
        if (!process.waitForStarted(3000))
            return false;
        if (!process.waitForFinished(5000) || process.exitCode() != 0)
            return false;

        m_pythonExecutable = candidate;
        m_pythonVersion = QString::fromUtf8(process.readAllStandardOutput()).trimmed().toStdString();
        if (m_pythonVersion.empty())
            m_pythonVersion = QString::fromUtf8(process.readAllStandardError()).trimmed().toStdString();
        return !m_pythonVersion.empty();
    }

    bool RuntimeManager::detectPythonVersion()
    {
        return !m_pythonVersion.empty();
    }
}