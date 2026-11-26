#pragma once

#include "PhysicsAsync.h"

struct AsyncCompatData {
	int calls;
	void* observed;
};

#ifdef __cplusplus
extern "C" {
#endif

PE_TaskId async_compat_submit(struct AsyncCompatData* data);

#ifdef __cplusplus
}
#endif
