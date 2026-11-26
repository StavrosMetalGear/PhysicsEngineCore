#include "quantum_c_api.h"
#include <cstddef>
#include <type_traits>
#include "baselines/quantum_records_msvc_x64.h"

#if !defined(_MSC_VER) || !defined(_M_X64)
#error This snapshot is only for the MSVC x64 ABI.
#endif

static_assert(sizeof(int) == 4 && sizeof(double) == 8, "MSVC x64 scalar sizes changed");
#define CHECK_RECORD(type, size, alignment) static_assert(std::is_standard_layout_v<type> && sizeof(type) == (size) && alignof(type) == (alignment), #type " layout changed");
#define CHECK_FIELD(type, field, offset, field_type) static_assert(offsetof(type, field) == (offset) && std::is_same_v<decltype(type::field), field_type>, #type "." #field " changed");
PE_QUANTUM_RECORD_BASELINE(CHECK_RECORD, CHECK_FIELD)
#undef CHECK_FIELD
#undef CHECK_RECORD
