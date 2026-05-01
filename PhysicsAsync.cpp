// ╔══════════════════════════════════════════════════════════════════════════════╗
//  PhysicsAsync.cpp — Thread-pool implementation for async task system
// ╚══════════════════════════════════════════════════════════════════════════════╝

#include "PhysicsAsync.h"

#include <thread>
#include <mutex>
#include <condition_variable>
#include <queue>
#include <vector>
#include <atomic>
#include <unordered_map>
#include <functional>

// ── Internal state ──────────────────────────────────────────────────────────

struct TaskEntry {
    PE_TaskFunc   func;
    void*         userData;
    PE_TaskId     id;
};

static std::vector<std::thread>              g_workers;
static std::queue<TaskEntry>                 g_taskQueue;
static std::mutex                            g_queueMutex;
static std::condition_variable               g_queueCV;
static std::atomic<bool>                     g_running{false};

static std::mutex                            g_completionMutex;
static std::condition_variable               g_completionCV;
static std::unordered_map<uint64_t, bool>    g_completed;

static std::atomic<uint64_t>                 g_nextId{1};
static std::atomic<int>                      g_pending{0};

// ── Worker loop ─────────────────────────────────────────────────────────────

static void workerLoop()
{
    while (true) {
        TaskEntry task;
        {
            std::unique_lock<std::mutex> lock(g_queueMutex);
            g_queueCV.wait(lock, [] { return !g_taskQueue.empty() || !g_running; });
            if (!g_running && g_taskQueue.empty()) return;
            task = g_taskQueue.front();
            g_taskQueue.pop();
        }

        // Execute
        if (task.func) {
            task.func(task.userData);
        }

        // Mark complete
        {
            std::lock_guard<std::mutex> lock(g_completionMutex);
            g_completed[task.id] = true;
        }
        g_completionCV.notify_all();
        g_pending.fetch_sub(1, std::memory_order_relaxed);
    }
}

// ── Public API ──────────────────────────────────────────────────────────────

extern "C" {

PHYSENG_API void pe_async_init(int numThreads)
{
    if (g_running) return;

    int n = numThreads;
    if (n <= 0) {
        n = static_cast<int>(std::thread::hardware_concurrency());
        if (n <= 0) n = 2;
    }

    g_running = true;
    g_workers.reserve(n);
    for (int i = 0; i < n; ++i) {
        g_workers.emplace_back(workerLoop);
    }
}

PHYSENG_API void pe_async_shutdown(void)
{
    if (!g_running) return;

    g_running = false;
    g_queueCV.notify_all();

    for (auto& w : g_workers) {
        if (w.joinable()) w.join();
    }
    g_workers.clear();

    {
        std::lock_guard<std::mutex> lock(g_completionMutex);
        g_completed.clear();
    }
    g_pending = 0;
}

PHYSENG_API PE_TaskId pe_submit_async(PE_TaskFunc func, void* userData)
{
    PE_TaskId id = g_nextId.fetch_add(1, std::memory_order_relaxed);

    {
        std::lock_guard<std::mutex> lock(g_completionMutex);
        g_completed[id] = false;
    }

    {
        std::lock_guard<std::mutex> lock(g_queueMutex);
        g_taskQueue.push({func, userData, id});
    }

    g_pending.fetch_add(1, std::memory_order_relaxed);
    g_queueCV.notify_one();
    return id;
}

PHYSENG_API int pe_task_ready(PE_TaskId id)
{
    std::lock_guard<std::mutex> lock(g_completionMutex);
    auto it = g_completed.find(id);
    if (it == g_completed.end()) return 1; // unknown task → treat as done
    return it->second ? 1 : 0;
}

PHYSENG_API void pe_task_wait(PE_TaskId id)
{
    std::unique_lock<std::mutex> lock(g_completionMutex);
    g_completionCV.wait(lock, [id] {
        auto it = g_completed.find(id);
        return it == g_completed.end() || it->second;
    });
}

PHYSENG_API int pe_tasks_pending(void)
{
    return g_pending.load(std::memory_order_relaxed);
}

} // extern "C"
