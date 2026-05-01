// ╔══════════════════════════════════════════════════════════════════════════════╗
//  PhysicsEngine.cpp — Engine metadata and module registry
// ╚══════════════════════════════════════════════════════════════════════════════╝

#include "PhysicsEngine.h"

static const char* ENGINE_VERSION = "1.0.0";

struct ModuleInfo {
    const char* name;
    int         simCount;
};

static const ModuleInfo g_modules[] = {
    { "QuantumCore",            50 },
    { "ClassicalMechanicsCore", 50 },
    { "ElectrodynamicsCore",    50 },
    { "FluidDynamicsCore",      50 },
    { "NuclearCore",            50 },
    { "OpticsCore",             50 },
    { "SolidStateCore",         50 },
    { "ThermodynamicsCore",     50 },
};

static const int g_moduleCount = sizeof(g_modules) / sizeof(g_modules[0]);

extern "C" {

PHYSENG_API const char* pe_version(void)
{
    return ENGINE_VERSION;
}

PHYSENG_API int pe_module_count(void)
{
    return g_moduleCount;
}

PHYSENG_API const char* pe_module_name(int index)
{
    if (index < 0 || index >= g_moduleCount) return nullptr;
    return g_modules[index].name;
}

PHYSENG_API int pe_module_sim_count(int index)
{
    if (index < 0 || index >= g_moduleCount) return 0;
    return g_modules[index].simCount;
}

} // extern "C"
