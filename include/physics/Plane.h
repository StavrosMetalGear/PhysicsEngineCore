#pragma once
#include <array>
#include <cmath>
#include "physics/Sphere.h"
#include "physics/AABB.h"

struct Plane {
    std::array<float, 3> normal; // Should be normalized
    float d; // Plane equation: normal.x * x + normal.y * y + normal.z * z + d = 0
};

// Signed distance from point to plane
inline float distance_to_plane(const Plane& plane, const std::array<float, 3>& point) {
    return plane.normal[0]*point[0] + plane.normal[1]*point[1] + plane.normal[2]*point[2] + plane.d;
}

// Sphere vs Plane intersection
inline bool intersects_sphere_plane(const Sphere& sphere, const Plane& plane) {
    float dist = distance_to_plane(plane, sphere.center);
    return std::fabs(dist) <= sphere.radius;
}

// AABB vs Plane intersection (returns true if AABB crosses the plane)
inline bool intersects_aabb_plane(const AABB& box, const Plane& plane) {
    // Compute projection interval radius of box onto plane normal
    float r = 0.0f;
    for (int i = 0; i < 3; ++i) {
        r += (box.max[i] - box.min[i]) * 0.5f * std::fabs(plane.normal[i]);
    }
    // Compute distance from box center to plane
    std::array<float, 3> c = box.center();
    float s = distance_to_plane(plane, c);
    return std::fabs(s) <= r;
}
