#pragma once

#include "PythonHost/PythonHostAPI.h"

#include <functional>

namespace PyHost
{
    PYTHONHOST_API void postToMainThread(std::function<void()> fn);
}
