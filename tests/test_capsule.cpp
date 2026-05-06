#include "physics/Capsule.h"
#include "physics/Sphere.h"
#include <iostream>

int main() {
    // Capsule vs Sphere
    Capsule cap1{{0,0,0}, {2,0,0}, 0.5f};
    Sphere s1{{1,0,0}, 0.5f};
    Sphere s2{{3,0,0}, 0.5f};
    std::cout << "cap1 intersects s1: " << (intersects_capsule_sphere(cap1, s1) != NoIntersection) << std::endl;
    std::cout << "cap1 intersects s2: " << (intersects_capsule_sphere(cap1, s2) != NoIntersection) << std::endl;

    // Capsule vs Capsule
    Capsule cap2{{1,1,0}, {3,1,0}, 0.5f};
    Capsule cap3{{5,0,0}, {7,0,0}, 0.5f};
    std::cout << "cap1 intersects cap2: " << intersects_capsule_capsule(cap1, cap2) << std::endl;
    std::cout << "cap1 intersects cap3: " << intersects_capsule_capsule(cap1, cap3) << std::endl;
    return 0;
}
