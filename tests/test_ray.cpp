#include "physics/Ray.h"
#include "physics/Sphere.h"
#include "physics/Plane.h"
#include <iostream>

int main() {
    Ray ray{{0, 0, 0}, {0, 1, 0}};
    Sphere sphere{{0, 5, 0}, 1.0f};
    float t0, t1;
    std::cout << "ray intersects sphere: " << intersects_ray_sphere(ray, sphere, t0, t1) << ", t0=" << t0 << ", t1=" << t1 << std::endl;

    Plane plane{{0, 1, 0}, -3};
    float t;
    std::cout << "ray intersects plane: " << intersects_ray_plane(ray, plane, t) << ", t=" << t << std::endl;
    return 0;
}
