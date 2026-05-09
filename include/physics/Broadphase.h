#pragma once
#include <vector>
#include <utility>
#include <algorithm>
#include "physics/AABB.h"

struct BroadphaseObject {
    int id;
    AABB box;
};

class Broadphase {
public:
    void addObject(int id, const AABB& box) {
        objects.push_back({id, box});
    }

    // Returns pairs of IDs whose AABBs potentially overlap
    std::vector<std::pair<int, int>> computePairs() {
        // Sort objects by min x
        std::vector<BroadphaseObject> sorted = objects;
        std::sort(sorted.begin(), sorted.end(), [](const BroadphaseObject& a, const BroadphaseObject& b) {
            return a.box.min[0] < b.box.min[0];
        });
        std::vector<std::pair<int, int>> pairs;
        for (size_t i = 0; i < sorted.size(); ++i) {
            for (size_t j = i + 1; j < sorted.size(); ++j) {
                if (sorted[j].box.min[0] > sorted[i].box.max[0]) break; // No overlap possible
                if (sorted[i].box.intersects(sorted[j].box)) {
                    pairs.emplace_back(sorted[i].id, sorted[j].id);
                }
            }
        }
        return pairs;
    }
private:
    std::vector<BroadphaseObject> objects;
};
