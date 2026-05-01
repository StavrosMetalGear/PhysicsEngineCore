#pragma once

// ╔══════════════════════════════════════════════════════════════════════════════╗
//  PhysicsCoupling.h — Cross-module bridge functions
//
//  Provides coupled computations that span multiple physics submodules.
//  Each function pulls data from two or more *Core libraries and returns
//  a unified result.
//
//  These are the "glue" functions that make the unified engine more than
//  the sum of its parts.
// ╚══════════════════════════════════════════════════════════════════════════════╝

#include "PhysicsEngine_export.h"

#ifdef __cplusplus
extern "C" {
#endif

// ═══════════════════════════════════════════════════════════════════════════════
//  1. FLUID + THERMODYNAMICS  (convective heat transfer, compressible flow)
// ═══════════════════════════════════════════════════════════════════════════════

// Nusselt number for forced convection over a flat plate (laminar).
// Combines fluid Re (from FluidDynamicsCore) with thermal Pr (ThermodynamicsCore).
//   Re = rho * U * L / mu,  Pr = cp * mu / k
//   Nu = 0.332 * Re^0.5 * Pr^(1/3)
PHYSENG_API double pe_nusselt_flat_plate(double rho, double U, double L,
                                         double mu, double cp, double k);

// Convective heat flux  q = h * (T_surface - T_fluid)  where h = Nu * k / L.
PHYSENG_API double pe_convective_heat_flux(double Nu, double k, double L,
                                           double T_surface, double T_fluid);

// ═══════════════════════════════════════════════════════════════════════════════
//  2. ELECTRODYNAMICS + OPTICS  (EM wave–matter interaction)
// ═══════════════════════════════════════════════════════════════════════════════

// Skin depth of an EM wave in a conductor:  delta = sqrt(2 / (mu0 * sigma * omega))
PHYSENG_API double pe_skin_depth(double sigma, double omega);

// Reflectance of a metal surface at normal incidence from the Drude model:
//   n + ik from plasma frequency and damping, then R = |(n-1+ik)/(n+1+ik)|^2
PHYSENG_API double pe_drude_reflectance(double omega_p, double gamma, double omega);

// ═══════════════════════════════════════════════════════════════════════════════
//  3. QUANTUM + SOLID STATE  (band structure, effective mass)
// ═══════════════════════════════════════════════════════════════════════════════

// Quantum confinement energy shift for an electron in a quantum dot,
// using the solid-state effective mass and the quantum ISW formula:
//   Delta E = pi^2 hbar^2 / (2 m* L^2)
PHYSENG_API double pe_quantum_dot_confinement(double mStar, double L);

// Fermi energy of a free electron gas (from SolidStateCore DOS + QuantumCore):
//   E_F = (hbar^2 / 2m) * (3 pi^2 n)^(2/3)
PHYSENG_API double pe_fermi_energy(double electronDensity, double mStar);

// ═══════════════════════════════════════════════════════════════════════════════
//  4. NUCLEAR + QUANTUM  (tunneling in nuclear reactions)
// ═══════════════════════════════════════════════════════════════════════════════

// Gamow peak energy for stellar fusion:
//   E_0 = (b * kT / 2)^(2/3)   where b = pi * alpha * Z1 * Z2 * sqrt(2 mu c^2)
PHYSENG_API double pe_gamow_peak(double T_kelvin, int Z1, int Z2, double mu);

// ═══════════════════════════════════════════════════════════════════════════════
//  5. CLASSICAL MECHANICS + FLUID  (drag, projectile in fluid)
// ═══════════════════════════════════════════════════════════════════════════════

// Terminal velocity of a sphere falling through a viscous fluid:
//   v_t = (2/9) * (rho_sphere - rho_fluid) * g * r^2 / mu
PHYSENG_API double pe_terminal_velocity(double rho_sphere, double rho_fluid,
                                         double radius, double mu);

// ═══════════════════════════════════════════════════════════════════════════════
//  6. THERMODYNAMICS + NUCLEAR  (reactor thermal output)
// ═══════════════════════════════════════════════════════════════════════════════

// Thermal power from fission:  P = fission_rate * energy_per_fission
PHYSENG_API double pe_fission_thermal_power(double fissionRate,
                                             double energyPerFission_MeV);

#ifdef __cplusplus
}
#endif
