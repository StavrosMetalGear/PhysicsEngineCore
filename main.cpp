// PhysicsEngineCore smoke test
#include "PhysicsEngine.h"
#include <cstdio>

int main() {
    printf("PhysicsEngineCore v%s — %d modules\n", pe_version(), pe_module_count());
    for (int i = 0; i < pe_module_count(); ++i) {
        printf("  [%d] %s (%d options)\n", i, pe_module_name(i), pe_module_sim_count(i));
    }

    // QuantumCore: 1D box energy
    QP_Handle qp = qp_create("electron", 9.1093837015e-31, 1e-10, 1);
    double E3 = qp_energy_1d_box(qp, 3);
    printf("Quantum: E3 (n=3, 1D box) = %.6e J\n", E3);
    qp_destroy(qp);

    // FluidDynamicsCore: Reynolds number
    double Re = fluid_reynolds_number(998.0, 1.5, 0.05, 1.002e-3);
    printf("Fluid: Re (water, U=1.5, L=0.05) = %.2f\n", Re);

    // ClassicalMechanicsCore: create and get mass
    CP_Handle cp = cp_create("ball", 1.0, 1.0, 3);
    printf("Classical: mass = %.2f kg\n", cp_getMass(cp));
    cp_destroy(cp);

    // NuclearCore: test API (binding energy for Fe-56)
    NP_Handle np = np_create();
    double be = np_binding_energy_semf(np, 26, 56); // Iron-56
    printf("Nuclear: BE (Fe-56) = %.2f MeV\n", be);
    np_destroy(np);

    // Async: test thread pool
    pe_async_init(2);
    volatile int result = 0;
    PE_TaskId tid = pe_submit_async([](void* arg) {
        int* p = (int*)arg; *p = 42;
    }, (void*)&result);
    pe_task_wait(tid);
    printf("Async: result = %d\n", result);
    pe_async_shutdown();

    // Coupling: Fermi energy
    double Ef = pe_fermi_energy(8.5e28, 9.109e-31);
    printf("Coupling: Fermi energy = %.3e J\n", Ef);

    printf("Smoke test complete.\n");
    return 0;
}
