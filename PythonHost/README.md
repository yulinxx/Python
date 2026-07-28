# Python Host Library (PythonHost.dll)

## 功能描述

Python宿主库，提供Python脚本嵌入执行、任务注册与执行、算法调度等功能，实现Python与C++的桥接。

本模块是 SanYi CAD 项目中 Python 集成框架的核心组件，负责在 C++ 应用中以统一的方式加载、调度和执行 Python 算法任务，支持子进程（Subprocess）与桥接（Bridge）两种运行模式。

## 目录结构

```
PythonHost/
├── Include/PythonHost/           # 公共 C++ API 头文件
│   ├── PythonHost.h              # 外观类（Facade），单例入口
│   ├── PythonHostAPI.h           # DLL 导出宏定义
│   ├── PythonHostTypes.h         # 核心类型与枚举定义
│   ├── PythonHostConfigUtil.h    # 配置构建工具
│   ├── TaskRegistry.h            # 任务注册表
│   ├── TaskExecutor.h            # 任务执行器
│   ├── RuntimeManager.h          # Python 运行时管理器
│   └── MainThreadDispatch.h      # 主线程分发接口
├── Src/                          # 实现文件
│   ├── PythonHost.cpp
│   ├── PythonHostConfigUtil.cpp
│   ├── TaskRegistry.cpp
│   ├── TaskExecutor.cpp
│   ├── RuntimeManager.cpp
│   └── MainThreadDispatch.cpp
└── CMakeLists.txt                # 构建配置
```

## 使用方法

### 1. 初始化与配置

```cpp
#include "PythonHost/PythonHost.h"
#include "PythonHost/PythonHostConfigUtil.h"

// 从应用目录自动构建配置（自动探测 Python 路径、tasks.json 等）
PyHost::Config cfg = PyHost::buildConfigFromApplicationDir(appDir, sourceRoot);

// 手动设置关键路径
cfg.pythonRoot      = "C:/Python311";          // Python 安装根目录
cfg.tasksConfigPath = "C:/MyApp/tasks.json";    // 任务配置文件路径
cfg.enableEmbedded  = false;                    // 是否启用嵌入式 Python
cfg.dispatchCallbacksOnMainThread = true;       // 回调是否在主线程分发

// 初始化
bool ok = PyHost::PythonHost::instance().initialize(cfg);
```

### 2. 创建与执行任务

```cpp
PyHost::TaskRequest req;
req.taskId      = "face.detect";      // 任务标识（需与 tasks.json 中对应）
req.inputJson   = R"({"image":"data/face.jpg"})";  // JSON 格式输入参数
req.timeoutMs   = 30000;              // 超时时间（毫秒）
req.cancellable = true;               // 是否可取消
req.priority    = PyHost::TaskPriority::Normal;

// 提交任务并接收回调
PyHost::TaskHandle handle = PyHost::PythonHost::instance().runTask(req,
    [](const PyHost::TaskResult& result) {
        if (result.ok) {
            // 成功：result.resultJson 包含算法输出
        } else {
            // 失败：result.error / result.message 包含错误信息
        }
    });
```

### 3. 取消任务

```cpp
PyHost::PythonHost::instance().cancel(handle);
```

### 4. 查询运行时信息

```cpp
PyHost::RuntimeInfo info = PyHost::PythonHost::instance().info();
// info.initialized        是否已初始化
// info.pythonExecutable   Python 可执行文件路径
// info.pythonRoot         Python 根目录
// info.pythonVersion      Python 版本号
// info.registeredTaskCount 已注册任务数量
```

### 5. 关闭

```cpp
PyHost::PythonHost::instance().shutdown();
```

### 6. 任务配置文件（tasks.json）

```json
{
    "echo": {
        "entry": "sanyi.tasks.echo",
        "mode": "subprocess",
        "timeoutMs": 10000
    },
    "face.detect": {
        "entry": "sanyi.tasks.face_detect",
        "mode": "subprocess",
        "timeoutMs": 60000
    }
}
```

## 设计框架

### 架构总览

```
┌─────────────────────────────────────────────┐
│             PythonHost (Facade)              │
│          单例入口 · 统一对外接口              │
├─────────────────────────────────────────────┤
│                                             │
│  ┌─────────────┐   ┌─────────────────┐    │
│  │ TaskRegistry │   │  TaskExecutor    │    │
│  │  任务注册表   │   │  任务执行器      │    │
│  └─────────────┘   └────────┬────────┘    │
│                              │              │
│                     ┌────────▼────────┐     │
│                     │  RuntimeManager  │    │
│                     │ 运行时管理器     │    │
│                     └─────────────────┘     │
│                                             │
│  ┌──────────────────────┐                  │
│  │ MainThreadDispatch   │                  │
│  │   主线程回调分发       │                  │
│  └──────────────────────┘                  │
│                                             │
├─────────────────────────────────────────────┤
│          Python Runtime (Python 3.11)        │
│    Subprocess Mode / Bridge Mode            │
└─────────────────────────────────────────────┘
```

### 核心设计模式

- **Facade 模式**：`PythonHost` 作为单例外观类，提供简洁统一的 API，屏蔽内部注册表、执行器、运行时管理等子系统的复杂性。

- **桥接模式**：支持 `Subprocess`（子进程）和 `Bridge`（嵌入式）两种执行模式，通过 `TaskMode` 枚举抽象，上层无需关心底层实现差异。

- **注册表模式**：`TaskRegistry` 通过 JSON 配置文件动态加载任务定义，支持算法的热插拔，新增算法只需修改配置即可。

- **策略模式**：`TaskPriority`（Low/Normal/High）与超时、可取消等策略灵活组合，适配不同任务的调度需求。

### 任务执行流程

1. **加载配置**：`TaskRegistry::load()` 解析 `tasks.json`，建立 `taskId → TaskDefinition` 映射表
2. **构建请求**：调用方构造 `TaskRequest`，指定 taskId、输入 JSON、超时等
3. **查找任务**：`TaskRegistry::lookup()` 根据 taskId 获取任务定义
4. **执行调度**：`TaskExecutor::run()` 根据任务定义的 mode 选择执行路径
   - Subprocess 模式：启动 Python 子进程，通过 stdin/stdout 进行 JSON 通信
   - Bridge 模式：直接在进程内调用（预留，Phase 3）
5. **回调通知**：任务完成后，通过 `MainThreadDispatch` 将结果回调分发至主线程

## API 概要

### 命名空间

```cpp
namespace PyHost { }
```

### 主要类型

| 类型 | 说明 |
|------|------|
| `TaskHandle` | 任务句柄（`uint64_t`），用于取消和状态查询 |
| `JsonString` | JSON 字符串类型别名（`std::string`） |
| `TaskRequest` | 任务请求结构：taskId、inputJson、timeoutMs 等 |
| `TaskResult` | 任务结果结构：ok、error、resultJson、metricsJson 等 |
| `TaskDefinition` | 任务定义结构：taskId、entry、mode、timeoutMs |
| `Config` | 宿主配置结构：pythonRoot、venvPath、tasksConfigPath 等 |
| `RuntimeInfo` | 运行时信息结构：initialized、pythonVersion 等 |
| `TaskCallback` | 任务回调类型：`std::function<void(const TaskResult&)>` |

### 枚举

| 枚举 | 值 | 说明 |
|------|----|------|
| `TaskError` | None, PythonNotFound, ScriptFailed, Timeout, Cancelled, InvalidInput, ProtocolError, UnknownTask, NotInitialized | 任务错误码 |
| `TaskMode` | Subprocess, Bridge | 任务执行模式 |
| `TaskPriority` | Low, Normal, High | 任务优先级 |

### 核心类方法

#### PythonHost（外观类）

| 方法 | 说明 |
|------|------|
| `static PythonHost& instance()` | 获取单例实例 |
| `bool initialize(const Config&)` | 初始化宿主，加载运行时与任务配置 |
| `void shutdown()` | 关闭宿主，释放资源 |
| `bool isInitialized() const` | 是否已初始化 |
| `TaskHandle runTask(const TaskRequest&, TaskCallback)` | 提交异步任务 |
| `bool cancel(TaskHandle)` | 取消正在执行的任务 |
| `RuntimeInfo info() const` | 获取运行时信息 |
| `const Config& config() const` | 获取当前配置 |

#### TaskRegistry（任务注册表）

| 方法 | 说明 |
|------|------|
| `bool load(const std::string& tasksConfigPath)` | 从 JSON 文件加载任务定义 |
| `void clear()` | 清空注册表 |
| `bool lookup(const std::string& taskId, TaskDefinition& out) const` | 按 ID 查找任务定义 |
| `int taskCount() const` | 已注册任务数量 |

#### TaskExecutor（任务执行器）

| 方法 | 说明 |
|------|------|
| `TaskHandle run(const TaskRequest&, const TaskDefinition&, TaskCallback)` | 执行任务 |
| `bool cancel(TaskHandle)` | 取消指定任务 |
| `bool isRunning(TaskHandle) const` | 查询任务是否正在运行 |

#### RuntimeManager（运行时管理器）

| 方法 | 说明 |
|------|------|
| `bool initialize(const Config&)` | 初始化 Python 运行时 |
| `void shutdown()` | 关闭运行时 |
| `bool isInitialized() const` | 是否已初始化 |
| `std::string pythonExecutable() const` | Python 可执行文件路径 |
| `std::string pythonVersion() const` | Python 版本号 |
| `std::string resolvePythonRoot() const` | 解析 Python 根目录 |
| `std::string resolveTasksConfigPath() const` | 解析任务配置路径 |

#### PythonHostConfigUtil（配置工具）

| 方法 | 说明 |
|------|------|
| `Config buildConfigFromApplicationDir(const std::string& appDir, const std::string& sourceRoot)` | 从应用目录自动构建配置 |

#### MainThreadDispatch（主线程分发）

| 方法 | 说明 |
|------|------|
| `void postToMainThread(std::function<void()> fn)` | 将函数调度至主线程执行 |

## 依赖库

### 必要依赖

| 库 | 说明 |
|----|------|
| **Python 3.11** | 嵌入/子进程执行的 Python 运行时环境 |
| **Qt Core** | 跨平台基础库，用于事件循环与主线程调度 |
| **Log** | SanYi 日志库，用于运行日志输出 |

### 可选依赖

| 库 | 说明 |
|----|------|
| **SanYi PyBindCore** | Phase 2 桥接层，提供 Python 侧 `sanyi` 包的 Facade API（`SANYI_PYBIND_CORE` 选项） |

### 依赖库安装方法

#### Python 3.11

1. 从 [Python 官网](https://www.python.org/downloads/release/python-3119/) 下载 Python 3.11.x 安装包
2. 安装时勾选 **"Add Python to PATH"**
3. 建议安装路径：`C:\Python311`（或项目指定的 `sourcePythonRoot`）
4. 验证安装：

```bash
python --version
# 应输出：Python 3.11.x
```

#### Qt Core

由项目顶层 CMake 统一管理，需在 CMake 配置中指定 Qt 安装路径（`CMAKE_PREFIX_PATH`）。

#### SanYi Log 库

作为项目内模块，构建时自动链接，无需单独安装。

## 构建配置

### CMake 选项

| 选项 | 默认值 | 说明 |
|------|--------|------|
| `SANYI_PYTHON_HOST` | `ON` | 是否构建 PythonHost 模块 |
| `SANYI_PYBIND_CORE` | `ON` | 是否构建 PyBindCore 桥接扩展 |
| `SANYI_PYTHON_EMBED` | `OFF` | 预留给进程内桥接模式（Phase 3） |

### CMake 构建示例

```bash
# 配置
cmake -S . -B build ^
    -DSANYI_PYTHON_HOST=ON ^
    -DSANYI_PYBIND_CORE=ON ^
    -DCMAKE_PREFIX_PATH="C:/Qt/6.x/msvc2019_64"

# 构建
cmake --build build --config Release
```

### 构建产物

- **Windows**：`PythonHost.dll`（Release）/ `PythonHost_d.dll`（Debug）
- 输出目录遵循 SanYi 项目统一的 `lib/<platform>/<config>/` 规范

### 编译特性

- C++17 标准（`cxx_std_17`）
- MSVC：`/utf-8`、`/FS`
- GCC/Clang：`-Wall -Wextra -Wpedantic`

## Python 工作进程协议

### 子进程模式通信协议

C++ 宿主通过以下方式启动 Python 工作进程：

```
python -m sanyi.runtime --root <PythonRoot>
```

**stdin 输入**（C++ → Python）：

```json
{
    "taskId": "face.detect",
    "traceId": "xxx-xxx-xxx",
    "input": {
        "image": "data/face.jpg"
    },
    "timeout": 30000,
    "cancellable": true
}
```

**stdout 输出**（Python → C++）：

```json
{
    "ok": true,
    "result": {
        "faces": [{ "x": 100, "y": 200, "w": 150, "h": 180 }]
    },
    "metrics": {
        "inference_ms": 45
    },
    "traceId": "xxx-xxx-xxx"
}
```

### 错误处理

当 Python 工作进程发生错误时，C++ 侧会接收到对应 `TaskError`：

| 错误码 | 触发条件 |
|--------|----------|
| `PythonNotFound` | 指定的 Python 可执行文件不存在 |
| `ScriptFailed` | Python 脚本运行时抛出异常 |
| `Timeout` | 任务执行超过 timeoutMs |
| `Cancelled` | 任务被用户主动取消 |
| `InvalidInput` | 输入 JSON 格式错误或字段缺失 |
| `ProtocolError` | 与工作进程的 JSON 通信协议异常 |
| `UnknownTask` | taskId 在注册表中未找到 |
| `NotInitialized` | 宿主尚未初始化即提交任务 |

## 版本信息

- **当前版本**：1.0.0
- **版本格式**：`MAJOR.MINOR.PATCH`
- **ABI 标识**：`PYTHONHOST_API` 宏控制（Windows 下使用 `__declspec(dllexport/dllimport)`）
- **C++ 标准**：C++17