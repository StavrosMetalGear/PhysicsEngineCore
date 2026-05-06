#include "physics/Triangle.h"
#include "physics/Sphere.h"
#include <iostream>

int main() {
    Triangle tri{{0,0,0}, {1,0,0}, {0,1,0}};
    Sphere s1{{0.3f,0.3f,0}, 0.1f};
    Sphere s2{{2,2,0}, 0.1f};
    std::cout << "s1 intersects tri: " << intersects_sphere_triangle(s1, tri) << std::endl;
    std::cout << "s2 intersects tri: " << intersects_sphere_triangle(s2, tri) << std::endl;
    return 0;
}
