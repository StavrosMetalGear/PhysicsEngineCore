#include "test_async_c_compat_client.h"

#include <iostream>

int main()
{
	AsyncCompatData data{0, nullptr};
	pe_async_init(1);
	const PE_TaskId task = async_compat_submit(&data);
	if (task == 0) {
		pe_async_shutdown();
		std::cerr << "FAIL: C callback submission\n";
		return 1;
	}
	pe_task_wait(task);
	const bool completed = pe_task_ready(task) == 1 && data.calls == 1 && data.observed == &data;
	pe_async_shutdown();
	if (!completed) {
		std::cerr << "FAIL: C callback completion or user data\n";
		return 1;
	}
	return 0;
}
