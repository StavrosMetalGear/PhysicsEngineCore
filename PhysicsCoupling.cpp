// ╔══════════════════════════════════════════════════════════════════════════════╗
//  PhysicsCoupling.cpp — Cross-module bridge implementations
// ╚══════════════════════════════════════════════════════════════════════════════╝

#ifndef _USE_MATH_DEFINES
#define _USE_MATH_DEFINES
#endif
#include <cmath>
#include <limits>
#include "PhysicsCoupling.h"

// Physical constants (SI)
static constexpr double HBAR      = 1.0545718e-34;   // J·s
static constexpr double MU0       = 1.2566370614e-6;  // H/m
static constexpr double KB        = 1.380649e-23;     // J/K
static constexpr double E_CHARGE  = 1.602176634e-19;  // C
static constexpr double C_LIGHT   = 2.99792458e8;     // m/s
static constexpr double ALPHA_FS  = 1.0 / 137.035999084;
static constexpr double G_ACCEL   = 9.80665;          // m/s^2

static double invalid_value()
{
    return std::numeric_limits<double>::quiet_NaN();
}

extern "C" {

// ─── 1. Fluid + Thermo ─────────────────────────────────────────────────────

PHYSENG_API double pe_nusselt_flat_plate(double rho, double U, double L,
                                         double mu, double cp, double k)
{
    if (!std::isfinite(rho) || !std::isfinite(U) || !std::isfinite(L) ||
        !std::isfinite(mu) || !std::isfinite(cp) || !std::isfinite(k) ||
        rho < 0.0 || U < 0.0 || L < 0.0 || mu <= 0.0 || cp < 0.0 || k <= 0.0) {
        return invalid_value();
    }
    double Re = rho * U * L / mu;
    double Pr = cp * mu / k;
    return 0.332 * std::sqrt(Re) * std::cbrt(Pr);
}

PHYSENG_API double pe_convective_heat_flux(double Nu, double k, double L,
                                           double T_surface, double T_fluid)
{
    if (!std::isfinite(Nu) || !std::isfinite(k) || !std::isfinite(L) ||
        !std::isfinite(T_surface) || !std::isfinite(T_fluid) ||
        Nu < 0.0 || k < 0.0 || L <= 0.0) {
        return invalid_value();
    }
    double h = Nu * k / L;
    return h * (T_surface - T_fluid);
}

// ─── 2. EM + Optics ────────────────────────────────────────────────────────

PHYSENG_API double pe_skin_depth(double sigma, double omega)
{
    if (!std::isfinite(sigma) || !std::isfinite(omega) || sigma <= 0.0 || omega <= 0.0) {
        return invalid_value();
    }
    return std::sqrt(2.0 / (MU0 * sigma * omega));
}

PHYSENG_API double pe_drude_reflectance(double omega_p, double gamma, double omega)
{
    if (!std::isfinite(omega_p) || !std::isfinite(gamma) || !std::isfinite(omega) ||
        omega_p < 0.0 || gamma < 0.0 || omega <= 0.0) {
        return invalid_value();
    }
    // Drude dielectric function:  eps = 1 - omega_p^2 / (omega^2 + i*gamma*omega)
    double w2 = omega * omega;
    double denom = w2 + gamma * gamma;
    double eps_r = 1.0 - (omega_p * omega_p * w2) / (w2 * denom + 1e-300);
    double eps_i = (omega_p * omega_p * gamma * omega) / (w2 * denom + 1e-300);

    // n + ik from eps = (n + ik)^2
    double absEps = std::sqrt(eps_r * eps_r + eps_i * eps_i);
    double n = std::sqrt((absEps + eps_r) / 2.0);
    double kk = std::sqrt((absEps - eps_r) / 2.0);

    // Normal incidence reflectance: R = |(n-1+ik)/(n+1+ik)|^2
    double num = (n - 1.0) * (n - 1.0) + kk * kk;
    double den = (n + 1.0) * (n + 1.0) + kk * kk;
    return num / (den + 1e-300);
}

// ─── 3. Quantum + Solid State ──────────────────────────────────────────────

PHYSENG_API double pe_quantum_dot_confinement(double mStar, double L)
{
    if (!std::isfinite(mStar) || !std::isfinite(L) || mStar <= 0.0 || L <= 0.0) {
        return invalid_value();
    }
    return (M_PI * M_PI * HBAR * HBAR) / (2.0 * mStar * L * L);
}

PHYSENG_API double pe_fermi_energy(double electronDensity, double mStar)
{
    if (!std::isfinite(electronDensity) || !std::isfinite(mStar) ||
        electronDensity < 0.0 || mStar <= 0.0) {
        return invalid_value();
    }
    double factor = 3.0 * M_PI * M_PI * electronDensity;
    return (HBAR * HBAR / (2.0 * mStar)) * std::pow(factor, 2.0 / 3.0);
}

// ─── 4. Nuclear + Quantum ──────────────────────────────────────────────────

PHYSENG_API double pe_gamow_peak(double T_kelvin, int Z1, int Z2, double mu)
{
    if (!std::isfinite(T_kelvin) || !std::isfinite(mu) || T_kelvin < 0.0 ||
        Z1 <= 0 || Z2 <= 0 || mu <= 0.0) {
        return invalid_value();
    }
    double kT = KB * T_kelvin;
    double b = M_PI * ALPHA_FS * Z1 * Z2 * std::sqrt(2.0 * mu * C_LIGHT * C_LIGHT);
    // E_0 = (b kT / 2)^(2/3)
    return std::pow(b * kT / 2.0, 2.0 / 3.0);
}

// ─── 5. Classical Mechanics + Fluid ────────────────────────────────────────

PHYSENG_API double pe_terminal_velocity(double rho_sphere, double rho_fluid,
                                         double radius, double mu)
{
    if (!std::isfinite(rho_sphere) || !std::isfinite(rho_fluid) ||
        !std::isfinite(radius) || !std::isfinite(mu) || rho_sphere < 0.0 ||
        rho_fluid < 0.0 || radius < 0.0 || mu <= 0.0) {
        return invalid_value();
    }
    return (2.0 / 9.0) * (rho_sphere - rho_fluid) * G_ACCEL * radius * radius / mu;
}

// ─── 6. Thermodynamics + Nuclear ───────────────────────────────────────────

PHYSENG_API double pe_fission_thermal_power(double fissionRate,
                                             double energyPerFission_MeV)
{
    if (!std::isfinite(fissionRate) || !std::isfinite(energyPerFission_MeV) ||
        fissionRate < 0.0 || energyPerFission_MeV < 0.0) {
        return invalid_value();
    }
    double energyPerFission_J = energyPerFission_MeV * 1.0e6 * E_CHARGE;
    return fissionRate * energyPerFission_J;
}

} // extern "C"
