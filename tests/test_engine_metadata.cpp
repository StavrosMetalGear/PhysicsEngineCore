#include "PhysicsEngine.h"

#include <iostream>
#include <string>

int main()
{
	static const char* expectedNames[] = {
		"QuantumCore",
		"ClassicalMechanicsCore",
		"ElectrodynamicsCore",
		"FluidDynamicsCore",
		"NuclearCore",
		"OpticsCore",
		"SolidStateCore",
		"ThermodynamicsCore"
	};

	if (pe_module_count() != 8) {
		std::cerr << "FAIL: expected eight modules\n";
		return 1;
	}

	if (std::string(pe_version()) != "1.0.0") {
		std::cerr << "FAIL: unexpected engine version\n";
		return 1;
	}

	const uint64_t expectedCapabilities =
		PE_CAPABILITY_METADATA | PE_CAPABILITY_ASYNC | PE_CAPABILITY_COUPLING |
		PE_CAPABILITY_QUANTUM | PE_CAPABILITY_CLASSICAL | PE_CAPABILITY_ELECTRODYNAMICS |
		PE_CAPABILITY_FLUID | PE_CAPABILITY_NUCLEAR | PE_CAPABILITY_OPTICS |
		PE_CAPABILITY_SOLID_STATE | PE_CAPABILITY_THERMODYNAMICS;
	if ((pe_capabilities() & expectedCapabilities) != expectedCapabilities) {
		std::cerr << "FAIL: missing compiled-in API capability\n";
		return 1;
	}

	for (int i = 0; i < pe_module_count(); ++i) {
		if (pe_module_name(i) == nullptr || std::string(pe_module_name(i)) != expectedNames[i]) {
			std::cerr << "FAIL: unexpected module name at index " << i << "\n";
			return 1;
		}
		if (pe_module_sim_count(i) != 50) {
			std::cerr << "FAIL: unexpected simulation count at index " << i << "\n";
			return 1;
		}
	}

	if (pe_module_name(-1) != nullptr || pe_module_name(pe_module_count()) != nullptr ||
		pe_module_sim_count(-1) != 0 || pe_module_sim_count(pe_module_count()) != 0) {
		std::cerr << "FAIL: invalid metadata index behavior\n";
		return 1;
	}

	std::cout << "PhysicsEngine metadata tests passed.\n";
	return 0;
}
