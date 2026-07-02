#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifdef PYTHONHOST_EXPORTS
#define PYTHONHOST_API __declspec(dllexport)
#else
#define PYTHONHOST_API __declspec(dllimport)
#endif
#elif defined(__GNUC__) || defined(__clang__) || defined(__APPLE__)
#ifdef PYTHONHOST_EXPORTS
#define PYTHONHOST_API __attribute__((visibility("default")))
#else
#define PYTHONHOST_API
#endif
#else
#define PYTHONHOST_API
#endif
