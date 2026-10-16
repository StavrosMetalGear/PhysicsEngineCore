#pragma once

// ╔══════════════════════════════════════════════════════════════════════════════╗
//  PhysicsAsync.h — Lightweight async task system for the unified engine
//
//  Allows submitting physics computations to a background thread pool and
//  polling for completion.  Designed for integration with game-engine
//  tick loops (Unreal Engine, etc.) where blocking the main thread is
//  unacceptable.
//
//  Usage:
//    pe_async_init(4);                          // 4 worker threads
//    PE_TaskId id = pe_submit_async(fn, arg);   // fire and forget
//    while (!pe_task_ready(id)) { /* game tick */ }
//    pe_async_shutdown();
// ╚══════════════════════════════════════════════════════════════════════════════╝

#include "PhysicsEngine_export.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Opaque task identifier
typedef uint64_t PE_TaskId;

// Task function signature:  void myTask(void* userData)
typedef void (*PE_TaskFunc)(void* userData);

// Initialize the process-global async system with the given number of worker
// threads. Call before submitting tasks. If numThreads <= 0, the implementation
// uses hardware_concurrency with a fallback. Repeated initialization is ignored.
PHYSENG_API void pe_async_init(int numThreads);

// Shut down all worker threads and release resources. The caller must coordinate
// shutdown with submitted work. The callback userData is never freed by the API.
PHYSENG_API void pe_async_shutdown(void);

// Submit a task for asynchronous execution. The callback and userData must
// remain valid until the callback finishes. Returns an opaque task ID.
PHYSENG_API PE_TaskId pe_submit_async(PE_TaskFunc func, void* userData);

// Returns 1 if the task has completed, 0 otherwise. Unknown IDs are currently
// treated as complete; callers should retain valid task IDs.
PHYSENG_API int pe_task_ready(PE_TaskId id);

// Block until the task completes.
PHYSENG_API void pe_task_wait(PE_TaskId id);

// Returns the number of tasks currently queued or running. The value is a
// snapshot and may change immediately after it is returned.
PHYSENG_API int pe_tasks_pending(void);

#ifdef __cplusplus
}
#endif
