#include "PhysicsAsync.h"

#include <atomic>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <vector>

namespace {

#define CHECK(condition, message) \
	do { \
		if (!(condition)) { \
			std::cerr << "FAIL: " << message << "\n"; \
			return 1; \
		} \
	} while (false)

void increment_task(void* data)
{
	static_cast<std::atomic<int>*>(data)->fetch_add(1, std::memory_order_relaxed);
}

void throwing_task(void*)
{
	throw std::runtime_error("integration test callback exception");
}

} // namespace

int main()
{
	pe_async_shutdown();

	const PE_TaskId beforeInit = pe_submit_async(increment_task, nullptr);
	CHECK(beforeInit == 0, "submission before initialization is rejected");
	CHECK(pe_task_ready(beforeInit) == 1, "rejected task is immediately complete");
	pe_task_wait(beforeInit);

	pe_async_init(4);
	pe_async_init(2);
	const PE_TaskId nullTask = pe_submit_async(nullptr, nullptr);
	CHECK(nullTask == 0, "null callback is rejected");
	CHECK(pe_task_ready(nullTask) == 1, "null callback task is immediately complete");

	std::atomic<int> completed{0};
	std::vector<PE_TaskId> tasks;
	tasks.reserve(512);
	for (int i = 0; i < 512; ++i) {
		const PE_TaskId task = pe_submit_async(increment_task, &completed);
		CHECK(task != 0, "stress task submission");
		tasks.push_back(task);
	}

	CHECK(pe_tasks_pending() >= 0, "pending task count is non-negative");
	for (const PE_TaskId task : tasks) {
		pe_task_wait(task);
		CHECK(pe_task_ready(task) == 1, "stress task completion");
	}
	CHECK(completed.load(std::memory_order_relaxed) == 512, "all stress callbacks executed");

	const PE_TaskId throwingTask = pe_submit_async(throwing_task, nullptr);
	CHECK(throwingTask != 0, "throwing task submission");
	pe_task_wait(throwingTask);
	CHECK(pe_task_ready(throwingTask) == 1, "throwing task reaches completion");

	pe_async_shutdown();
	CHECK(pe_tasks_pending() == 0, "pending count after shutdown");
	pe_async_shutdown();

	std::thread initializerA([] {
		for (int i = 0; i < 8; ++i) pe_async_init(2);
	});
	std::thread initializerB([] {
		for (int i = 0; i < 8; ++i) pe_async_init(2);
	});
	initializerA.join();
	initializerB.join();

	std::atomic<int> shutdownCompleted{0};
	for (int i = 0; i < 128; ++i) {
		CHECK(pe_submit_async(increment_task, &shutdownCompleted) != 0,
			  "shutdown-drain task submission");
	}
	pe_async_shutdown();
	CHECK(shutdownCompleted.load(std::memory_order_relaxed) == 128,
		  "shutdown drains queued tasks");
	CHECK(pe_tasks_pending() == 0, "pending count after drained shutdown");

	std::cout << "Async stress assertions passed.\n";
	return 0;
}
