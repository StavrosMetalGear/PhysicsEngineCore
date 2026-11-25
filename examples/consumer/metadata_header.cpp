#include <PhysicsMetadata.h>
#include <type_traits>

static_assert(std::is_same_v<decltype(&pe_version), const char* (*)()>);
static_assert(std::is_same_v<decltype(&pe_capabilities), uint64_t (*)()>);
