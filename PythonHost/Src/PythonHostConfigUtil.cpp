#include "PythonHost/PythonHostConfigUtil.h"

#include <filesystem>

namespace fs = std::filesystem;

namespace PyHost
{
    namespace
    {
        std::string joinPath(const std::string& base, const std::string& child)
        {
            return (fs::path(base) / child).string();
        }
    }

    Config buildConfigFromApplicationDir(
        const std::string& applicationDir,
        const std::string& sourceRoot)
    {
        Config config;
        config.pythonRoot = joinPath(applicationDir, "Python");
        config.venvPath = joinPath(config.pythonRoot, "venv");
        config.extensionPath = joinPath(config.pythonRoot, "sanyi");
        config.tasksConfigPath = joinPath(config.pythonRoot, "tasks.json");
        config.sourcePythonRoot = sourceRoot.empty()
            ? std::string()
            : joinPath(sourceRoot, "Python");
        config.enableEmbedded = false;
        config.dispatchCallbacksOnMainThread = true;
        config.workerThreads = 2;
        return config;
    }
}