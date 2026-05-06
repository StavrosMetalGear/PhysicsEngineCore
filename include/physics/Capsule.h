#pragma once
#include <array>

#include <cmath>
#include <limits>
#include "physics/Sphere.h"

struct Capsule {
    std::array<float, 3> pointA; // One endpoint
    std::array<float, 3> pointB; // Other endpoint
    float radius;
};

// Helper: squared distance between two points
inline float dist2(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    float d2 = 0.0f;
    for (int i = 0; i < 3; ++i) {
        float d = a[i] - b[i];
        d2 += d * d;
    }
    return d2;
}

// Helper: closest point on segment ab to point p
inline std::array<float, 3> closestPointOnSegment(const std::array<float, 3>& a, const std::array<float, 3>& b, const std::array<float, 3>& p) {
    std::array<float, 3> ab, ap;
    for (int i = 0; i < 3; ++i) {
        ab[i] = b[i] - a[i];
        ap[i] = p[i] - a[i];
    }
    float t = 0.0f;
    float ab2 = 0.0f;
    for (int i = 0; i < 3; ++i) ab2 += ab[i] * ab[i];
    if (ab2 > 1e-6f) {
        for (int i = 0; i < 3; ++i) t += ab[i] * ap[i];
        t /= ab2;
        t = std::fmax(0.0f, std::fmin(1.0f, t));
    }
    std::array<float, 3> cp;
    for (int i = 0; i < 3; ++i) cp[i] = a[i] + t * ab[i];
    return cp;
}

// Capsule vs Sphere intersection
enum CapsuleSphereResult { NoIntersection, Touching, Overlapping };
inline CapsuleSphereResult intersects_capsule_sphere(const Capsule& capsule, const Sphere& sphere) {
    auto cp = closestPointOnSegment(capsule.pointA, capsule.pointB, sphere.center);
    float rSum = capsule.radius + sphere.radius;
    float d2 = dist2(cp, sphere.center);
    if (d2 > rSum * rSum) return NoIntersection;
    if (std::fabs(d2 - rSum * rSum) < 1e-6f) return Touching;
    return Overlapping;
}

// Capsule vs Capsule intersection (returns true if they overlap)
inline bool intersects_capsule_capsule(const Capsule& c1, const Capsule& c2) {
    // Compute closest points between segments
    // (for brevity, use a simple sampling approach; for production, use full segment-segment distance)
    float minDist2 = std::numeric_limits<float>::max();
    for (float t1 = 0; t1 <= 1.0f; t1 += 0.25f) {
        std::array<float, 3> p1;
        for (int i = 0; i < 3; ++i) p1[i] = c1.pointA[i] + t1 * (c1.pointB[i] - c1.pointA[i]);
        for (float t2 = 0; t2 <= 1.0f; t2 += 0.25f) {
            std::array<float, 3> p2;
            for (int i = 0; i < 3; ++i) p2[i] = c2.pointA[i] + t2 * (c2.pointB[i] - c2.pointA[i]);
            float d2 = dist2(p1, p2);
            if (d2 < minDist2) minDist2 = d2;
        }
    }
    float rSum = c1.radius + c2.radius;
    return minDist2 <= rSum * rSum;
}
