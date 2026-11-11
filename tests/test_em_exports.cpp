#include "PhysicsEngine.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <locale>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <vector>

namespace {

namespace fs = std::filesystem;

class TemporaryDirectory {
public:
	TemporaryDirectory()
	{
		std::random_device random;
		const auto root = fs::temp_directory_path();
		for (int attempt = 0; attempt < 100; ++attempt) {
			auto candidate = root / ("physics-engine-exports-" + std::to_string(random()) + "-" + std::to_string(random()));
			std::error_code error;
			if (fs::create_directory(candidate, error)) {
				path = candidate;
				return;
			}
			if (error && error != std::errc::file_exists) {
				throw fs::filesystem_error("Could not create temporary directory", candidate, error);
			}
		}
		throw std::runtime_error("Could not allocate a unique temporary directory");
	}

	~TemporaryDirectory()
	{
		std::error_code error;
		fs::remove_all(path, error);
	}

	fs::path path;
};

bool verifyCsv(const fs::path& path, const std::string& header,
			   const std::function<std::vector<double>(int)>& expected)
{
	std::ifstream input(path);
	input.imbue(std::locale::classic());
	std::string line;
	if (!std::getline(input, line) || line != header) {
		std::cerr << "FAIL: missing file or unexpected CSV header: " << path << "\n";
		return false;
	}
	for (int row = 0; row < 3; ++row) {
		if (!std::getline(input, line)) {
			std::cerr << "FAIL: missing CSV row " << row << ": " << path << "\n";
			return false;
		}
		std::istringstream fields(line);
		fields.imbue(std::locale::classic());
		const auto values = expected(row);
		std::string field;
		for (std::size_t column = 0; column < values.size(); ++column) {
			if (!std::getline(fields, field, ',')) {
				std::cerr << "FAIL: missing CSV column: " << path << "\n";
				return false;
			}
			std::istringstream number(field);
			number.imbue(std::locale::classic());
			double actual = 0.0;
			if (!(number >> actual) || !(number >> std::ws).eof() || !std::isfinite(actual) ||
				std::abs(actual - values[column]) > 1.0e-5 * std::max(std::abs(values[column]), 1.0e-25)) {
				std::cerr << "FAIL: invalid CSV value in " << path << " row " << row
						  << " column " << column << ": " << field << " expected " << values[column] << "\n";
				return false;
			}
		}
		if (std::getline(fields, field, ',') || (!line.empty() && line.back() == ',')) {
			std::cerr << "FAIL: extra CSV column: " << path << "\n";
			return false;
		}
	}
	if (std::getline(input, line) || input.bad()) {
		std::cerr << "FAIL: extra row or read error: " << path << "\n";
		return false;
	}
	return true;
}

bool testExports(const fs::path& root)
{
	constexpr double pi = 3.14159265358979323846;
	constexpr double eps0 = 8.854187817e-12;
	constexpr double mu0 = 1.2566370614e-6;
	constexpr double ke = 8.9875517873681764e9;
	constexpr double charge = 1.0e-9;
	EM_Handle em = em_create("csv-exports", 2.5, 1.0, 5.8e7);
	if (!em) {
		std::cerr << "FAIL: Electrodynamics handle creation\n";
		return false;
	}
	struct HandleOwner {
		EM_Handle handle;
		~HandleOwner() { em_destroy(handle); }
	} owner{em};
	bool ok = true;
	auto verify = [&](const char* name, const char* header, const std::function<std::vector<double>(int)>& expected) {
		if (!verifyCsv(root / name, header, expected)) ok = false;
	};
	auto filename = [&](const char* name) { return (root / name).string(); };

	em_export_coulomb_csv(em, filename("coulomb.csv").c_str(), charge, 3);
	verify("coulomb.csv", "r,E,V,U(q1=q)", [&](int i) {
		const double r = 0.01 + i;
		return std::vector<double>{r, ke * charge / (r * r), ke * charge / r, ke * charge * charge / r};
	});
	em_export_gauss_csv(em, filename("gauss.csv").c_str(), charge, 0.2, 3);
	verify("gauss.csv", "r,E", [&](int i) {
		const double r = 0.001 + 0.2 * i;
		const double e = r < 0.2 ? charge * r / (4.0 * pi * 2.5 * eps0 * 0.008)
								  : charge / (4.0 * pi * 2.5 * eps0 * r * r);
		return std::vector<double>{r, e};
	});
	em_export_potential_csv(em, filename("potential.csv").c_str(), charge, 0.2, 3);
	verify("potential.csv", "z,V_point,V_ring,V_shell", [&](int i) {
		const double z = 0.001 + 0.25 * i;
		return std::vector<double>{z, ke * charge / z, ke * charge / std::sqrt(0.04 + z * z),
								   ke * charge / (z < 0.2 ? 0.2 : z)};
	});
	em_export_capacitance_csv(em, filename("capacitance.csv").c_str(), 0.02, 3);
	verify("capacitance.csv", "d,C", [&](int i) {
		const double d = 0.0001 + 0.005 * i;
		return std::vector<double>{d, 2.5 * eps0 * 0.02 / d};
	});
	em_export_dielectric_csv(em, filename("dielectric.csv").c_str(), 100.0, 3);
	verify("dielectric.csv", "chi_e,P,D", [&](int i) {
		const double chi = 0.01 + 5.0 * i;
		return std::vector<double>{chi, eps0 * chi * 100.0, 2.5 * eps0 * 100.0};
	});
	em_export_resistance_csv(em, filename("resistance.csv").c_str(), 2.0e-6, 3);
	verify("resistance.csv", "L,R", [](int i) {
		const double length = 0.01 + 5.0 * i;
		return std::vector<double>{length, 2.0 * length};
	});
	em_export_rc_csv(em, filename("rc.csv").c_str(), 12.0, 1000.0, 2.0e-6, 3);
	verify("rc.csv", "t,V_charge,I_discharge", [](int i) {
		const double time = 0.005 * i;
		return std::vector<double>{time, 12.0 * (1.0 - std::exp(-time / 0.002)),
								   0.012 * std::exp(-time / 0.002)};
	});
	em_export_biot_savart_csv(em, filename("biot.csv").c_str(), 4.0, 0.2, 3);
	verify("biot.csv", "z,B_on_axis", [&](int i) {
		const double z = -0.5 + 0.5 * i;
		return std::vector<double>{z, mu0 * 4.0 * 0.04 / (2.0 * std::pow(0.04 + z * z, 1.5))};
	});
	em_export_ampere_csv(em, filename("ampere.csv").c_str(), 4.0, 0.2, 3);
	verify("ampere.csv", "r,B", [&](int i) {
		const double r = 0.0001 + 0.3 * i;
		const double b = r < 0.2 ? mu0 * 4.0 * r / (2.0 * pi * 0.04)
								  : mu0 * 4.0 / (2.0 * pi * r);
		return std::vector<double>{r, b};
	});
	em_export_vector_potential_csv(em, filename("vector-potential.csv").c_str(), 4.0, 0.2, 3);
	verify("vector-potential.csv", "r,A_wire,A_phi_solenoid", [&](int i) {
		const double r = 0.001 + 0.3 * i;
		const double solenoid = r < 0.2 ? 0.25 * r : 0.25 * 0.04 / r;
		return std::vector<double>{r, -mu0 * 4.0 * std::log(r) / (2.0 * pi), solenoid};
	});
	em_export_faraday_csv(em, filename("faraday.csv").c_str(), 2.0, 0.5, 3);
	verify("faraday.csv", "v,EMF", [](int i) {
		const double v = 50.0 * i;
		return std::vector<double>{v, v};
	});
	em_export_inductance_csv(em, filename("inductance.csv").c_str(), 10.0, 0.2, 3);
	verify("inductance.csv", "l,L", [&](int i) {
		const double length = 0.01 + 0.5 * i;
		return std::vector<double>{length, mu0 * 100.0 * 0.2 * length};
	});
	em_export_lr_csv(em, filename("lr.csv").c_str(), 12.0, 4.0, 2.0, 3);
	verify("lr.csv", "t,I_growth,I_decay", [](int i) {
		const double t = 1.25 * i;
		return std::vector<double>{t, 3.0 * (1.0 - std::exp(-2.0 * t)), 3.0 * std::exp(-2.0 * t)};
	});
	em_export_lc_csv(em, filename("lc.csv").c_str(), 2.0, 1.0, 1.0, 3);
	verify("lc.csv", "t,Q", [&](int i) {
		const double t = 5.0 * pi * i;
		return std::vector<double>{t, i == 1 ? -2.0 : 2.0};
	});
	em_export_rlc_csv(em, filename("rlc.csv").c_str(), 2.0, 0.2, 1.0, 1.0, 3);
	verify("rlc.csv", "t,Q", [&](int i) {
		const double t = 10.0 * pi * i;
		return std::vector<double>{t, 2.0 * std::exp(-0.1 * t) * std::cos(std::sqrt(0.99) * t)};
	});
	em_export_ac_csv(em, filename("ac.csv").c_str(), 2.0, 1.0, 1.0, 3);
	verify("ac.csv", "f,Z_mag,PowerFactor", [&](int i) {
		const double omega = i == 0 ? 0.1 : (i == 1 ? 1.0 : 10.0);
		const double impedance = std::sqrt(4.0 + std::pow(omega - 1.0 / omega, 2.0));
		return std::vector<double>{omega / (2.0 * pi), impedance, 2.0 / impedance};
	});
	em_export_maxwell_diff_csv(em, filename("maxwell-diff.csv").c_str(), 3);
	verify("maxwell-diff.csv", "rho,divE", [&](int i) {
		const double rho = (i - 1) * 1.0e-5;
		return std::vector<double>{rho, rho / (2.5 * eps0)};
	});
	em_export_maxwell_int_csv(em, filename("maxwell-int.csv").c_str(), 3);
	verify("maxwell-int.csv", "Q,Flux", [&](int i) {
		const double q = (i - 1) * 1.0e-5;
		return std::vector<double>{q, q / (2.5 * eps0)};
	});
	const double speed = 299792458.0 / std::sqrt(2.5);
	em_export_poynting_csv(em, filename("poynting.csv").c_str(), 100.0, 3);
	verify("poynting.csv", "E0,I,u_E,u_B", [&](int i) {
		const double e = 100.0 * i;
		return std::vector<double>{e, 0.5 * 2.5 * eps0 * speed * e * e,
			0.5 * 2.5 * eps0 * e * e, 0.5 * e * e / (speed * speed * mu0)};
	});
	em_export_rect_waveguide_csv(em, filename("rect-waveguide.csv").c_str(), 0.2, 0.1, 3);
	verify("rect-waveguide.csv", "f,v_phase,v_group", [&](int i) {
		const double cutoff = speed / 0.4;
		const double factor = 1.01 + 1.5 * i;
		const double ratio = std::sqrt(1.0 - 1.0 / (factor * factor));
		return std::vector<double>{cutoff * factor, speed / ratio, speed * ratio};
	});
	em_export_retarded_csv(em, filename("retarded.csv").c_str(), charge, 3);
	verify("retarded.csv", "cosTheta,V_LW", [&](int i) {
		const double cosine = (i - 1) * 0.99;
		return std::vector<double>{cosine, ke * charge / (1.0 - 0.5 * cosine)};
	});
	em_export_electric_multipole_csv(em, filename("multipole.csv").c_str(), charge, 2.0e-9, 3.0e-9, 3);
	verify("multipole.csv", "r,V_mono,V_dipole,V_quad", [&](int i) {
		const double r = 0.01 + 0.25 * i;
		return std::vector<double>{r, ke * charge / r, ke * 2.0e-9 / (r * r),
			ke * 3.0e-9 / (2.0 * r * r * r)};
	});
	em_export_skin_effect_csv(em, filename("skin.csv").c_str(), 3);
	verify("skin.csv", "f,delta,Rs", [&](int i) {
		const double f = i == 0 ? 1.0e3 : (i == 1 ? 1.0e6 : 1.0e9);
		const double omega = 2.0 * pi * f;
		return std::vector<double>{f, std::sqrt(2.0 / (mu0 * 5.8e7 * omega)),
			std::sqrt(mu0 * omega / (2.0 * 5.8e7))};
	});
	em_export_image_charge_csv(em, filename("image.csv").c_str(), charge, 0.2, 3);
	verify("image.csv", "y,V", [&](int i) {
		const double y = 0.001 + 0.05 * i;
		return std::vector<double>{y, ke * charge * (1.0 / (0.2 - y) - 1.0 / (0.2 + y))};
	});
	em_export_separation_csv(em, filename("separation.csv").c_str(), 0.2, 2.0, 3);
	verify("separation.csv", "r,V_sphere,V_cylinder", [](int i) {
		const double r = 0.202 + 0.4 * i;
		return std::vector<double>{r, -2.0 * r + 2.0 * 0.008 / (r * r), -2.0 * r + 2.0 * 0.04 / r};
	});
	em_export_waveguide_dispersion_csv(em, filename("waveguide.csv").c_str(), 2.0, 3);
	verify("waveguide.csv", "omega_ratio,v_phase_over_c,v_group_over_c", [](int i) {
		const double ratio = 1.01 + 2.0 * i;
		const double group = std::sqrt(1.0 - 1.0 / (ratio * ratio));
		return std::vector<double>{ratio, 1.0 / group, group};
	});
	em_export_transmission_line_csv(em, filename("transmission-line.csv").c_str(), 50.0, 75.0, 3);
	verify("transmission-line.csv", "ZL,Gamma,VSWR", [](int i) {
		const double load = 1.0 + 100.0 * i;
		return std::vector<double>{load, (load - 50.0) / (load + 50.0),
			std::max(load, 50.0) / std::min(load, 50.0)};
	});
	em_export_polarization_csv(em, filename("polarization.csv").c_str(), 1.0, 3);
	verify("polarization.csv", "theta,I", [&](int i) {
		const double theta = i * pi / 2.0;
		return std::vector<double>{theta, std::pow(std::cos(theta), 2)};
	});
	em_export_magnetization_csv(em, filename("magnetization.csv").c_str(), 0.5, 3);
	verify("magnetization.csv", "x,Langevin,Brillouin", [](int i) {
		const double x = 0.01 + 5.0 * i;
		return std::vector<double>{x, 1.0 / std::tanh(x) - 1.0 / x, std::tanh(x)};
	});
	em_export_total_reflection_csv(em, filename("total-reflection.csv").c_str(), 1.5, 1.0, 3);
	verify("total-reflection.csv", "y,E_evanescent", [&](int i) {
		const double theta = std::asin(1.0 / 1.5) + 0.1;
		const double decay = 2.0 * pi / 500.0e-9 * std::sqrt(2.25 * std::pow(std::sin(theta), 2) - 1.0);
		return std::vector<double>{2.5 * i / decay, std::exp(-2.5 * i)};
	});
	em_export_fresnel_csv(em, filename("fresnel.csv").c_str(), 1.0, 1.5, 3);
	verify("fresnel.csv", "theta_i,Rs,Rp,Ts,Tp", [&](int i) {
		const double theta = (pi / 2.0 - 0.001) * i / 2.0;
		const double incidentCosine = std::cos(theta);
		const double transmittedCosine = std::sqrt(1.0 - std::pow(std::sin(theta) / 1.5, 2));
		const double rs = std::pow((incidentCosine - 1.5 * transmittedCosine) / (incidentCosine + 1.5 * transmittedCosine), 2);
		const double rp = std::pow((1.5 * incidentCosine - transmittedCosine) / (1.5 * incidentCosine + transmittedCosine), 2);
		return std::vector<double>{theta, rs, rp, 1.0 - rs, 1.0 - rp};
	});
	em_export_antenna_csv(em, filename("antenna.csv").c_str(), 3);
	verify("antenna.csv", "theta,pattern", [&](int i) {
		const double theta = 0.01 + (pi - 0.02) * i / 2.0;
		return std::vector<double>{theta, std::pow(std::cos(pi * std::cos(theta) / 2.0) / std::sin(theta), 2)};
	});
	const double frequency = 1.0e9;
	em_export_em_wave_csv(em, filename("em-wave.csv").c_str(), 2.0, frequency, 3);
	verify("em-wave.csv", "z,E,B", [&](int i) {
		const double z = 1.5 * speed * i / frequency;
		const double electric = i == 1 ? -2.0 : 2.0;
		return std::vector<double>{z, electric, electric / speed};
	});
	const double penetration = 2.0e-7;
	em_export_meissner_csv(em, filename("meissner.csv").c_str(), 0.5, penetration, 3);
	verify("meissner.csv", "x,B", [&](int i) {
		return std::vector<double>{2.5 * penetration * i, 0.5 * std::exp(-2.5 * i)};
	});
	return ok;
}

}

int main()
{
	try {
		TemporaryDirectory directory;
		return testExports(directory.path) ? 0 : 1;
	} catch (const std::exception& error) {
		std::cerr << "FAIL: CSV export test setup: " << error.what() << "\n";
		return 1;
	}
}
