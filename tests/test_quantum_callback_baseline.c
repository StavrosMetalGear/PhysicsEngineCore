#include "quantum_c_api.h"

typedef double (*ExpectedPotentialFunc)(double, double);
typedef double (*ExpectedBohrSommerfeld)(QP_Handle, ExpectedPotentialFunc, double, double, double, int, int);
typedef double (*ExpectedTunnelingProbability)(QP_Handle, ExpectedPotentialFunc, double, double, double, double, int);

_Static_assert(_Generic((QP_PotentialFunc)0, ExpectedPotentialFunc: 1, default: 0), "QP_PotentialFunc signature changed");
_Static_assert(_Generic(&qp_wkb_bohr_sommerfeld, ExpectedBohrSommerfeld: 1, default: 0), "qp_wkb_bohr_sommerfeld signature changed");
_Static_assert(_Generic(&qp_wkb_tunneling_probability, ExpectedTunnelingProbability: 1, default: 0), "qp_wkb_tunneling_probability signature changed");
