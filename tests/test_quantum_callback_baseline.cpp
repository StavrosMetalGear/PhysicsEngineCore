#include "quantum_c_api.h"
#include <type_traits>

using ExpectedPotentialFunc = double (*)(double, double);
using ExpectedBohrSommerfeld = double (*)(QP_Handle, ExpectedPotentialFunc, double, double, double, int, int);
using ExpectedTunnelingProbability = double (*)(QP_Handle, ExpectedPotentialFunc, double, double, double, double, int);

static_assert(std::is_same_v<QP_PotentialFunc, ExpectedPotentialFunc>, "QP_PotentialFunc signature changed");
static_assert(std::is_same_v<decltype(&qp_wkb_bohr_sommerfeld), ExpectedBohrSommerfeld>, "qp_wkb_bohr_sommerfeld signature changed");
static_assert(std::is_same_v<decltype(&qp_wkb_tunneling_probability), ExpectedTunnelingProbability>, "qp_wkb_tunneling_probability signature changed");
