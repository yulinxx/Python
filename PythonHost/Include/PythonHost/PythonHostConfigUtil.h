#pragma once

#include "PythonHost/PythonHostAPI.h"
#include "PythonHost/PythonHostTypes.h"

namespace PyHost
{
    PYTHONHOST_API Config buildConfigFromApplicationDir(
        const std::string& applicationDir, const std::string& sourceRoot = std::string());
}
