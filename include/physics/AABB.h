#pragma once
#include <array>
#include <algorithm>
#include <cmath>

struct AABB {
    std::array<float, 3> min; // Minimum x, y, z
    std::array<float, 3> max; // Maximum x, y, z

    // Check if this AABB intersects with another
    bool intersects(const AABB& other) const {
        for (int i = 0; i < 3; ++i) {
            if (max[i] < other.min[i] || min[i] > other.max[i])
                return false;
        }
        return true;
    }

    // Compute the center of the AABB
    std::array<float, 3> center() const {
        return { (min[0] + max[0]) * 0.5f, (min[1] + max[1]) * 0.5f, (min[2] + max[2]) * 0.5f };
    }

    // Compute the extent (half-size) of the AABB
    std::array<float, 3> extent() const {
        return { (max[0] - min[0]) * 0.5f, (max[1] - min[1]) * 0.5f, (max[2] - min[2]) * 0.5f };
    }
};

// Utility function for collision response: compute minimum translation vector
inline std::array<float, 3> computeMTV(const AABB& a, const AABB& b) {
    std::array<float, 3> mtv = {0, 0, 0};
    float minOverlap = std::numeric_limits<float>::max();
    int axis = -1;
    for (int i = 0; i < 3; ++i) {
        float overlap = std::min(a.max[i], b.max[i]) - std::max(a.min[i], b.min[i]);
        if (overlap < minOverlap) {
            minOverlap = overlap;
            axis = i;
        }
    }
    if (axis >= 0 && minOverlap > 0) {
        mtv[axis] = (a.center()[axis] < b.center()[axis] ? -minOverlap : minOverlap);
    }
    return mtv;
}
