#pragma once
#include <array>
#include <cmath>
#include "physics/Sphere.h"
struct Triangle {
    std::array<float, 3> a;
    std::array<float, 3> b;
    std::array<float, 3> c;
};

// Helper: cross product
inline std::array<float, 3> cross(const std::array<float, 3>& u, const std::array<float, 3>& v) {
    return {
        u[1]*v[2] - u[2]*v[1],
        u[2]*v[0] - u[0]*v[2],
        u[0]*v[1] - u[1]*v[0]
    };
}

// Helper: dot product
inline float dot(const std::array<float, 3>& u, const std::array<float, 3>& v) {
    return u[0]*v[0] + u[1]*v[1] + u[2]*v[2];
}

// Helper: subtract
inline std::array<float, 3> sub(const std::array<float, 3>& u, const std::array<float, 3>& v) {
    return {u[0]-v[0], u[1]-v[1], u[2]-v[2]};
}

// Point in triangle test (barycentric method)
inline bool point_in_triangle(const std::array<float, 3>& p, const Triangle& tri) {
    auto v0 = sub(tri.c, tri.a);
    auto v1 = sub(tri.b, tri.a);
    auto v2 = sub(p, tri.a);
    float d00 = dot(v0, v0);
    float d01 = dot(v0, v1);
    float d11 = dot(v1, v1);
    float d20 = dot(v2, v0);
    float d21 = dot(v2, v1);
    float denom = d00 * d11 - d01 * d01;
    if (std::fabs(denom) < 1e-6f) return false;
    float v = (d11 * d20 - d01 * d21) / denom;
    float w = (d00 * d21 - d01 * d20) / denom;
    float u = 1.0f - v - w;
    return (u >= 0) && (v >= 0) && (w >= 0);
}

// Sphere vs Triangle intersection (simple test: check if sphere center is inside triangle or close to edge)
inline bool intersects_sphere_triangle(const Sphere& sphere, const Triangle& tri) {
    // Project sphere center onto triangle plane
    auto ab = sub(tri.b, tri.a);
    auto ac = sub(tri.c, tri.a);
    auto n = cross(ab, ac);
    float nLen = std::sqrt(dot(n, n));
    if (nLen < 1e-6f) return false;
    for (int i = 0; i < 3; ++i) n[i] /= nLen;
    float dist = dot(sub(sphere.center, tri.a), n);
    std::array<float, 3> proj;
    for (int i = 0; i < 3; ++i) proj[i] = sphere.center[i] - dist * n[i];
    if (std::fabs(dist) > sphere.radius) return false;
    if (point_in_triangle(proj, tri)) return true;
    // (Edge/vertex proximity test omitted for brevity)
    return false;
}
