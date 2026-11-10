#include "PythonHost/PythonHost.h"

#include <cstdio>
#include <filesystem>

// ============================================================
//  PythonHost 使用示例
//  编译后执行 AprPythonHostExample.exe
//
//  演示 PythonHost 框架的基本流程：
//    initialize(Config) -> info() -> shutdown()
//
//  子进程模式（enableEmbedded=false）：宿主环境需要可用的 Python 解释器。
//  本机没有 Python 时 initialize 会返回 false，示例仍然正常退出 ——
//  它演示的是 API 用法，不依赖解释器一定可用。
// ============================================================

int main()
{
    std::printf("=== AprPythonHostExample ===\n");

    PyHost::Config config;
    config.pythonRoot = std::filesystem::current_path().string();
    config.enableEmbedded = false;  // 子进程模式
    config.workerThreads = 2;

    PyHost::PythonHost& host = PyHost::PythonHost::instance();
    const bool ok = host.initialize(config);
    std::printf("initialize -> %s\n", ok ? "OK" : "FAILED");

    if (ok)
    {
        const PyHost::RuntimeInfo info = host.info();
        std::printf("initialized : %d\n", (int)info.initialized);
        std::printf("python      : %s (%s)\n",
            info.pythonExecutable.c_str(), info.pythonVersion.c_str());
        std::printf("pythonRoot  : %s\n", info.pythonRoot.c_str());
        std::printf("tasks       : %d registered\n", info.registeredTaskCount);

        host.shutdown();
        std::printf("shutdown done, isInitialized = %d\n", (int)host.isInitialized());
    }
    else
    {
        std::printf("Python runtime unavailable in this environment; API demo only.\n");
    }

    return 0;
}
