#pragma once

#include "PythonHost/PythonHostTypes.h"

#include <string>

namespace PyHost
{
    class RuntimeManager
    {
    public:
        bool initialize(const Config& config);
        void shutdown();

        bool isInitialized() const;
        std::string pythonExecutable() const;
        std::string pythonRoot() const;
        std::string pythonVersion() const;

        std::string resolvePythonRoot() const;
        std::string resolveTasksConfigPath() const;

    private:
        bool probePythonExecutable(const std::string& candidate);
        bool detectPythonVersion();

        Config m_config;
        std::string m_pythonExecutable;
        std::string m_pythonVersion;
        bool m_initialized = false;
    };
}
