#pragma once

// Snapshot of the pinned Quantum C records with the MSVC x64 default ABI.
#define PE_QUANTUM_RECORD_BASELINE(RECORD, FIELD) \
	RECORD(QP_EnergyLevel2D, 16, 8) \
	FIELD(QP_EnergyLevel2D, nx, 0, int) \
	FIELD(QP_EnergyLevel2D, ny, 4, int) \
	FIELD(QP_EnergyLevel2D, energy, 8, double) \
	RECORD(QP_EnergyLevel3D, 24, 8) \
	FIELD(QP_EnergyLevel3D, nx, 0, int) \
	FIELD(QP_EnergyLevel3D, ny, 4, int) \
	FIELD(QP_EnergyLevel3D, nz, 8, int) \
	FIELD(QP_EnergyLevel3D, energy, 16, double) \
	RECORD(QP_ZeemanLevel, 48, 8) \
	FIELD(QP_ZeemanLevel, n, 0, int) \
	FIELD(QP_ZeemanLevel, l, 4, int) \
	FIELD(QP_ZeemanLevel, j, 8, double) \
	FIELD(QP_ZeemanLevel, mj, 16, double) \
	FIELD(QP_ZeemanLevel, E_noField, 24, double) \
	FIELD(QP_ZeemanLevel, E_withField, 32, double) \
	FIELD(QP_ZeemanLevel, g_j, 40, double) \
	RECORD(QP_HyperfineLevel, 24, 8) \
	FIELD(QP_HyperfineLevel, F, 0, double) \
	FIELD(QP_HyperfineLevel, mF, 8, double) \
	FIELD(QP_HyperfineLevel, E, 16, double)
