#include "PhysicsAsync.h"
#include <stdio.h>

struct TaskData {
	int calls;
	int value;
	void* observed;
};

static void record_task(void* userData)
{
	struct TaskData* data = (struct TaskData*)userData;
	data->calls += 1;
	data->value = 42;
	data->observed = userData;
}

#define CHECK(condition, message) \
	do { \
		if (!(condition)) { \
			fprintf(stderr, "FAIL: %s\n", message); \
			failed = 1; \
			goto cleanup; \
		} \
	} while (0)

int main(void)
{
	int failed = 0;
	struct TaskData task = {0, 0, NULL};
	struct TaskData drained[16] = {0};
	PE_TaskId taskId = 0;

	CHECK(pe_submit_async(record_task, &task) == 0, "submission before initialization");
	CHECK(pe_task_ready(0) == 1, "zero task ID is immediately complete");
	pe_task_wait(0);
	CHECK(task.calls == 0, "rejected submission did not run");

	pe_async_init(2);
	CHECK(pe_submit_async(NULL, &task) == 0, "null callback is rejected");
	taskId = pe_submit_async(record_task, &task);
	CHECK(taskId != 0, "valid callback is accepted");
	pe_task_wait(taskId);
	CHECK(pe_task_ready(taskId) == 1, "waited task is complete");
	CHECK(task.calls == 1 && task.value == 42 && task.observed == &task,
		"C callback received caller-owned data and completed once");

	for (int i = 0; i < 16; ++i) {
		CHECK(pe_submit_async(record_task, &drained[i]) != 0, "drain task is accepted");
	}
	pe_async_shutdown();
	for (int i = 0; i < 16; ++i) {
		CHECK(drained[i].calls == 1 && drained[i].value == 42 &&
			drained[i].observed == &drained[i], "shutdown drained C callbacks");
	}
	CHECK(pe_tasks_pending() == 0, "shutdown clears pending count");
	CHECK(pe_task_ready(taskId) == 1, "cleared task ID is treated as complete");
	pe_task_wait(taskId);

cleanup:
	pe_async_shutdown();
	return failed;
}
