#include "test_async_c_compat_client.h"

static void record_compat_task(void* userData)
{
	struct AsyncCompatData* data = (struct AsyncCompatData*)userData;
	data->calls += 1;
	data->observed = userData;
}

PE_TaskId async_compat_submit(struct AsyncCompatData* data)
{
	return pe_submit_async(record_compat_task, data);
}
