#pragma once
#include <array>
#include <cmath>

struct Sphere {
    std::array<float, 3> center;
    float radius;

    // Check if this sphere intersects with another sphere
    bool intersects(const Sphere& other) const {
        float distSq = 0.0f;
        for (int i = 0; i < 3; ++i) {
            float d = center[i] - other.center[i];
            distSq += d * d;
        }
        float rSum = radius + other.radius;
        return distSq <= rSum * rSum;
    }
};

#include "physics/AABB.h"
// Check if an AABB intersects with a Sphere
inline bool intersects_aabb_sphere(const AABB& box, const Sphere& sphere) {
    float distSq = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float v = sphere.center[i];
        if (v < box.min[i]) distSq += (box.min[i] - v) * (box.min[i] - v);
        else if (v > box.max[i]) distSq += (v - box.max[i]) * (v - box.max[i]);
    }
    return distSq <= sphere.radius * sphere.radius;
}
