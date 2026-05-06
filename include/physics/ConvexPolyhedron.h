#pragma once
#include <vector>
#include <array>
#include <cmath>

struct ConvexPolyhedron {
    std::vector<std::array<float, 3>> vertices;
    std::vector<std::array<int, 3>> faces; // Each face is a triangle (indices into vertices)
};

// Helper: dot product
inline float dot3(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return a[0]*b[0] + a[1]*b[1] + a[2]*b[2];
}

// Helper: subtract
inline std::array<float, 3> sub3(const std::array<float, 3>& a, const std::array<float, 3>& b) {
    return {a[0]-b[0], a[1]-b[1], a[2]-b[2]};
}

// Support function for GJK: find farthest point in direction d
inline std::array<float, 3> support(const ConvexPolyhedron& poly, const std::array<float, 3>& d) {
    float maxDot = -std::numeric_limits<float>::infinity();
    std::array<float, 3> best = poly.vertices[0];
    for (const auto& v : poly.vertices) {
        float dp = dot3(v, d);
        if (dp > maxDot) {
            maxDot = dp;
            best = v;
        }
    }
    return best;
}

// (Simple GJK intersection stub for two convex polyhedra)
inline bool intersects_convex_polyhedron(const ConvexPolyhedron& a, const ConvexPolyhedron& b) {
    // For brevity, this is a stub. Full GJK implementation is complex.
    // Here, just check if any vertex of a is inside b's bounding box and vice versa.
    // Replace with full GJK/EPA for production use.
    for (const auto& va : a.vertices) {
        float minB[3] = {INFINITY, INFINITY, INFINITY};
        float maxB[3] = {-INFINITY, -INFINITY, -INFINITY};
        for (const auto& vb : b.vertices) {
            for (int i = 0; i < 3; ++i) {
                if (vb[i] < minB[i]) minB[i] = vb[i];
                if (vb[i] > maxB[i]) maxB[i] = vb[i];
            }
        }
        bool inside = true;
        for (int i = 0; i < 3; ++i) {
            if (va[i] < minB[i] || va[i] > maxB[i]) inside = false;
        }
        if (inside) return true;
    }
    for (const auto& vb : b.vertices) {
        float minA[3] = {INFINITY, INFINITY, INFINITY};
        float maxA[3] = {-INFINITY, -INFINITY, -INFINITY};
        for (const auto& va : a.vertices) {
            for (int i = 0; i < 3; ++i) {
                if (va[i] < minA[i]) minA[i] = va[i];
                if (va[i] > maxA[i]) maxA[i] = va[i];
            }
        }
        bool inside = true;
        for (int i = 0; i < 3; ++i) {
            if (vb[i] < minA[i] || vb[i] > maxA[i]) inside = false;
        }
        if (inside) return true;
    }
    return false;
}
