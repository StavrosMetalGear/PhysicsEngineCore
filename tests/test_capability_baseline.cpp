#include "PhysicsMetadata.h"
#include "baselines/capability_snapshot.h"

#define CHECK_CAPABILITY(name, value) static_assert((name) == (value), #name " capability value changed");
PE_CAPABILITY_BASELINE(CHECK_CAPABILITY)
#undef CHECK_CAPABILITY
