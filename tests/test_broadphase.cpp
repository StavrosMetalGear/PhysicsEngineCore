#include "physics/Broadphase.h"
#include <iostream>

int main() {
    Broadphase broadphase;
    broadphase.addObject(1, {{0,0,0}, {2,2,2}});
    broadphase.addObject(2, {{1,1,1}, {3,3,3}});
    broadphase.addObject(3, {{4,4,4}, {5,5,5}});
    broadphase.addObject(4, {{2.5f,2.5f,2.5f}, {4,4,4}});

    auto pairs = broadphase.computePairs();
    for (const auto& p : pairs) {
        std::cout << "Potential overlap: " << p.first << " and " << p.second << std::endl;
    }
    return 0;
}
