#include "PhysicsMetadata.h"
#include "PhysicsAsync.h"
#include "PhysicsCoupling.h"
#include "baselines/engine_api_snapshot.h"

#define CHECK_ENGINE_SIGNATURE(name, result, args) _Static_assert(_Generic(&(name), result (*) args: 1, default: 0), #name " signature changed");

PE_ENGINE_SIGNATURE_BASELINE(CHECK_ENGINE_SIGNATURE)

#undef CHECK_ENGINE_SIGNATURE
