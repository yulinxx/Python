#pragma once

#include "PythonHost/PythonHostTypes.h"

#include <unordered_map>

namespace PyHost
{
    class TaskRegistry
    {
    public:
        bool load(const std::string& tasksConfigPath);
        void clear();

        bool lookup(const std::string& taskId, TaskDefinition& outDefinition) const;
        int taskCount() const;

    private:
        std::unordered_map<std::string, TaskDefinition> m_tasks;
    };
}  // namespace PyHost
