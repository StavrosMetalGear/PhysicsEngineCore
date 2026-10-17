#include "QuantumParticle.h"

#include <cmath>
#include <fstream>

double QuantumParticle::computeDeltaPotentialWavefunction(int n, double x, double V0) {
	(void)n;
	const double hbar = 1.0545718e-34;
	const double kappa = mass * V0 / (hbar * hbar);
	const double normalization = std::sqrt(kappa);
	return normalization * std::exp(-kappa * std::fabs(x));
}

void QuantumParticle::exportDeltaPotentialTimeEvolutionCSV(const std::string& filename,
															double V0,
															int numPoints,
															double tMax) {
	std::ofstream out(filename);
	out << "x,t,psi\n";

	const double hbar = 1.0545718e-34;
	const double kappa = mass * V0 / (hbar * hbar);
	const double normalization = std::sqrt(kappa);
	const double xMax = 1e-9;
	const double dx = 2 * xMax / numPoints;
	const int numT = 10;
	const double dt = tMax / numT;

	for (int it = 0; it <= numT; ++it) {
		const double t = it * dt;
		for (int i = 0; i < numPoints; ++i) {
			const double x = -xMax + i * dx;
			const double psi = normalization * std::exp(-kappa * std::fabs(x));
			out << x << "," << t << "," << psi << "\n";
		}
	}
}