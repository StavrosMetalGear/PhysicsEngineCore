#include <PhysicsMetadata.h>

int metadata_header_check(void)
{
	return pe_version() != 0 && pe_module_count() == 8 &&
		(pe_capabilities() & PE_CAPABILITY_METADATA) != 0;
}
