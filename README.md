# PythonHost

SanYi CAD Python integration framework (Phase 0/1).

## Layout

- `PythonHost/Include/PythonHost/` — public C++ API
- `PythonHost/Src/` — runtime, task registry, subprocess executor
- `../Python/` — deployable `sanyi` Python package and `tasks.json`

## C++ usage

```cpp
#include "PythonHost/PythonHost.h"
#include "PythonHost/PythonHostConfigUtil.h"

PyHost::Config cfg = PyHost::buildConfigFromApplicationDir(appDir, CMAKE_SOURCE_DIR);
PyHost::PythonHost::instance().initialize(cfg);

PyHost::TaskRequest req;
req.taskId = "echo";
req.inputJson = R"({"message":"hello"})";

PyHost::PythonHost::instance().runTask(req, [](const PyHost::TaskResult& result) {
    // callback runs on Qt main thread when dispatchCallbacksOnMainThread=true
});
```

## Python worker protocol

C++ launches:

```text
python -m sanyi.runtime --root <PythonRoot>
```

and writes one JSON object to stdin. The worker returns one JSON object on stdout.

## Tasks

Register tasks in `Python/tasks.json`. Implement handlers under `Python/sanyi/tasks/`.

Built-in scaffold tasks:

- `echo` — integration test
- `face.detect` — face detection placeholder (replace with real ML backend)

## PyBindCore facade (Phase 2)

When `_sanyi_core.pyd` is built (`SANYI_PYBIND_CORE=ON`), import via:

```python
import sanyi
doc = sanyi.Document.create()
```

See `PyBindCore/README.md` for facade API details.

## CMake options

- `SANYI_PYTHON_HOST` — build and link PythonHost (default ON)
- `SANYI_PYBIND_CORE` — build PyBindCore facade extension (default ON)
- `SANYI_PYTHON_EMBED` — reserved for in-process bridge (Phase 3)
