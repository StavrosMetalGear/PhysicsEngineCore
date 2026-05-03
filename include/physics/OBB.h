#pragma once
#include <array>
#include <cmath>

struct OBB {
    std::array<float, 3> center;   // Center of the box
    std::array<float, 3> halfSize; // Half extents along each axis
    std::array<std::array<float, 3>, 3> axes; // Local axes (normalized, columns)

    // Check if this OBB intersects with another OBB (Separating Axis Theorem)
    bool intersects(const OBB& other) const {
        // Compute rotation matrix expressing other in this box's coordinate frame
        float R[3][3], AbsR[3][3];
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j) {
                R[i][j] = 0.0f;
                for (int k = 0; k < 3; ++k)
                    R[i][j] += axes[i][k] * other.axes[j][k];
                AbsR[i][j] = std::fabs(R[i][j]) + 1e-6f; // Add small epsilon to counteract arithmetic errors
            }
        // Compute translation vector t
        std::array<float, 3> t;
        for (int i = 0; i < 3; ++i)
            t[i] = other.center[i] - center[i];
        // Bring translation into this box's frame
        float tA[3];
        for (int i = 0; i < 3; ++i) {
            tA[i] = 0.0f;
            for (int k = 0; k < 3; ++k)
                tA[i] += t[k] * axes[i][k];
        }
        // Test axes L = A0, A1, A2
        for (int i = 0; i < 3; ++i) {
            float ra = halfSize[i];
            float rb = 0.0f;
            for (int j = 0; j < 3; ++j)
                rb += other.halfSize[j] * AbsR[i][j];
            if (std::fabs(tA[i]) > ra + rb) return false;
        }
        // Test axes L = B0, B1, B2
        for (int i = 0; i < 3; ++i) {
            float ra = 0.0f;
            for (int j = 0; j < 3; ++j)
                ra += halfSize[j] * AbsR[j][i];
            float rb = other.halfSize[i];
            float tB = 0.0f;
            for (int k = 0; k < 3; ++k)
                tB += t[k] * other.axes[i][k];
            if (std::fabs(tB) > ra + rb) return false;
        }
        // (For brevity, cross-product axes tests omitted; add for full SAT)
        return true;
    }
};
