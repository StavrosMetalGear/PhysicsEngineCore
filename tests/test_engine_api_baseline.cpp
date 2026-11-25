#include "PhysicsMetadata.h"
#include "PhysicsAsync.h"
#include "PhysicsCoupling.h"
#include "baselines/engine_api_snapshot.h"
#include <type_traits>

#define CHECK_ENGINE_SIGNATURE(name, result, args) static_assert(std::is_same_v<decltype(&(name)), result (*) args>, #name " signature changed");

PE_ENGINE_SIGNATURE_BASELINE(CHECK_ENGINE_SIGNATURE)

#undef CHECK_ENGINE_SIGNATURE
