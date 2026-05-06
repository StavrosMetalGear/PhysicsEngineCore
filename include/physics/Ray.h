#pragma once
#include <array>
#include <cmath>
#include "physics/Sphere.h"
#include "physics/Plane.h"

struct Ray {
    std::array<float, 3> origin;
    std::array<float, 3> direction; // Should be normalized
};

// Ray vs Sphere intersection (returns true if intersects, t0/t1 are intersection distances)
inline bool intersects_ray_sphere(const Ray& ray, const Sphere& sphere, float& t0, float& t1) {
    std::array<float, 3> L;
    for (int i = 0; i < 3; ++i) L[i] = sphere.center[i] - ray.origin[i];
    float tca = 0.0f;
    for (int i = 0; i < 3; ++i) tca += L[i] * ray.direction[i];
    float d2 = 0.0f;
    for (int i = 0; i < 3; ++i) d2 += L[i] * L[i];
    d2 -= tca * tca;
    float r2 = sphere.radius * sphere.radius;
    if (d2 > r2) return false;
    float thc = std::sqrt(r2 - d2);
    t0 = tca - thc;
    t1 = tca + thc;
    return true;
}

// Ray vs Plane intersection (returns true if intersects, t is intersection distance)
inline bool intersects_ray_plane(const Ray& ray, const Plane& plane, float& t) {
    float denom = 0.0f;
    for (int i = 0; i < 3; ++i) denom += plane.normal[i] * ray.direction[i];
    if (std::fabs(denom) < 1e-6f) return false; // Parallel
    float num = 0.0f;
    for (int i = 0; i < 3; ++i) num += plane.normal[i] * ray.origin[i];
    t = -(num + plane.d) / denom;
    return t >= 0.0f;
}
