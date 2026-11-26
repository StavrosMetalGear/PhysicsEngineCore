#include "quantum_c_api.h"
#include <stddef.h>
#include "baselines/quantum_records_msvc_x64.h"

#if !defined(_MSC_VER) || !defined(_M_X64)
#error This snapshot is only for the MSVC x64 ABI.
#endif

_Static_assert(sizeof(int) == 4 && sizeof(double) == 8, "MSVC x64 scalar sizes changed");
#define CHECK_RECORD(type, size, alignment) _Static_assert(sizeof(type) == (size) && _Alignof(type) == (alignment), #type " layout changed");
#define CHECK_FIELD(type, field, offset, field_type) _Static_assert(offsetof(type, field) == (offset) && _Generic(((type*)0)->field, field_type: 1, default: 0), #type "." #field " changed");
PE_QUANTUM_RECORD_BASELINE(CHECK_RECORD, CHECK_FIELD)
#undef CHECK_FIELD
#undef CHECK_RECORD
