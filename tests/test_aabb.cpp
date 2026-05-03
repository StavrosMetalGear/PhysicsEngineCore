#include "physics/AABB.h"
#include <iostream>

int main() {
    AABB box1{{0, 0, 0}, {2, 2, 2}};
    AABB box2{{1, 1, 1}, {3, 3, 3}};
    AABB box3{{3, 3, 3}, {4, 4, 4}};

    std::cout << "box1 intersects box2: " << box1.intersects(box2) << std::endl;
    std::cout << "box1 intersects box3: " << box1.intersects(box3) << std::endl;

    if (box1.intersects(box2)) {
        auto mtv = computeMTV(box1, box2);
        std::cout << "MTV: (" << mtv[0] << ", " << mtv[1] << ", " << mtv[2] << ")\n";
    }
    return 0;
}
