#include "physics/Narrowphase.h"
#include <iostream>

// Example collision functions
bool aabb_vs_aabb(const Shape& a, const Shape& b) {
    return std::get<AABB>(a.data).intersects(std::get<AABB>(b.data));
}
bool sphere_vs_sphere(const Shape& a, const Shape& b) {
    return std::get<Sphere>(a.data).intersects(std::get<Sphere>(b.data));
}
bool aabb_vs_sphere(const Shape& a, const Shape& b) {
    return intersects_aabb_sphere(std::get<AABB>(a.data), std::get<Sphere>(b.data));
}

int main() {
    Narrowphase np;
    np.registerFunc(ShapeType::AABB, ShapeType::AABB, aabb_vs_aabb);
    np.registerFunc(ShapeType::Sphere, ShapeType::Sphere, sphere_vs_sphere);
    np.registerFunc(ShapeType::AABB, ShapeType::Sphere, aabb_vs_sphere);

    Shape s1{ShapeType::AABB, AABB{{0,0,0},{2,2,2}}};
    Shape s2{ShapeType::AABB, AABB{{1,1,1},{3,3,3}}};
    Shape s3{ShapeType::Sphere, Sphere{{1,1,1}, 1.0f}};
    Shape s4{ShapeType::Sphere, Sphere{{5,5,5}, 1.0f}};

    std::cout << "AABB vs AABB: " << np.test(s1, s2) << std::endl;
    std::cout << "AABB vs Sphere: " << np.test(s1, s3) << std::endl;
    std::cout << "Sphere vs Sphere: " << np.test(s3, s4) << std::endl;
    return 0;
}
