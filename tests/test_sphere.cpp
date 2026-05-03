#include "physics/AABB.h"
#include "physics/Sphere.h"
#include <iostream>

int main() {
    // Sphere vs Sphere
    Sphere s1{{0, 0, 0}, 1.0f};
    Sphere s2{{1.5f, 0, 0}, 1.0f};
    Sphere s3{{3, 0, 0}, 1.0f};
    std::cout << "s1 intersects s2: " << s1.intersects(s2) << std::endl;
    std::cout << "s1 intersects s3: " << s1.intersects(s3) << std::endl;

    // AABB vs Sphere
    AABB box{{-1, -1, -1}, {1, 1, 1}};
    Sphere s4{{0.5f, 0.5f, 0.5f}, 0.5f};
    Sphere s5{{2, 2, 2}, 0.5f};
    std::cout << "box intersects s4: " << intersects(box, s4) << std::endl;
    std::cout << "box intersects s5: " << intersects(box, s5) << std::endl;
    return 0;
}
