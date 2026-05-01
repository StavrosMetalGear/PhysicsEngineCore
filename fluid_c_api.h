#pragma once

// ╔══════════════════════════════════════════════════════════════════════════════╗
//  fluid_c_api.h — Header for the FluidDynamicsCore C API
//
//  Declares the extern "C" functions defined in FluidDynamicsCore's
//  fluid_c_api.cpp.  Uses the same FLUID_API macro from FluidExport.h.
// ╚══════════════════════════════════════════════════════════════════════════════╝

#include "FluidExport.h"

#ifdef __cplusplus
extern "C" {
#endif

FLUID_API double fluid_hydrostatic_pressure(double rho, double g, double h, double p0);

FLUID_API double fluid_bernoulli_velocity(double p1, double p2, double rho,
                                           double h1, double h2, double g);

FLUID_API double fluid_reynolds_number(double rho, double U, double L, double mu);

FLUID_API double fluid_speed_of_sound(double gamma, double R_specific, double T);

FLUID_API double fluid_mach_number(double v, double c);

#ifdef __cplusplus
}
#endif
