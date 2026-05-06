#include "physics/Plane.h"
#include "physics/Sphere.h"
#include "physics/AABB.h"
#include <iostream>

int main() {
    Plane plane{{0, 1, 0}, -1}; // y = 1
    Sphere s1{{0, 2, 0}, 1.0f};
    Sphere s2{{0, 3, 0}, 1.0f};
    std::cout << "s1 intersects plane: " << intersects_sphere_plane(s1, plane) << std::endl;
    std::cout << "s2 intersects plane: " << intersects_sphere_plane(s2, plane) << std::endl;

    AABB box1{{-1, 0, -1}, {1, 2, 1}};
    AABB box2{{-1, 3, -1}, {1, 4, 1}};
    std::cout << "box1 intersects plane: " << intersects_aabb_plane(box1, plane) << std::endl;
    std::cout << "box2 intersects plane: " << intersects_aabb_plane(box2, plane) << std::endl;
    return 0;
}
