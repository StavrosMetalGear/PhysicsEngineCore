#include "physics/Narrowphase.h"
#include "physics/OBB.h"
#include "physics/Capsule.h"
#include "physics/Plane.h"
#include "physics/Triangle.h"
#include "physics/ConvexPolyhedron.h"
#include <iostream>

#define CHECK(condition, message) \
    do { \
        if (!(condition)) { \
            std::cerr << "FAIL: " << message << "\n"; \
            return 1; \
        } \
    } while (false)

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
bool obb_vs_obb(const Shape& a, const Shape& b) {
    return std::get<OBB>(a.data).intersects(std::get<OBB>(b.data));
}
bool capsule_vs_capsule(const Shape& a, const Shape& b) {
    return intersects_capsule_capsule(std::get<Capsule>(a.data), std::get<Capsule>(b.data));
}
bool sphere_vs_plane(const Shape& a, const Shape& b) {
    return intersects_sphere_plane(std::get<Sphere>(a.data), std::get<Plane>(b.data));
}
bool aabb_vs_plane(const Shape& a, const Shape& b) {
    return intersects_aabb_plane(std::get<AABB>(a.data), std::get<Plane>(b.data));
}
bool sphere_vs_triangle(const Shape& a, const Shape& b) {
    return intersects_sphere_triangle(std::get<Sphere>(a.data), std::get<Triangle>(b.data));
}
bool convex_vs_convex(const Shape& a, const Shape& b) {
    return intersects_convex_polyhedron(std::get<ConvexPolyhedron>(a.data), std::get<ConvexPolyhedron>(b.data));
}

int main() {
    Narrowphase np;
    np.registerFunc(ShapeType::AABB, ShapeType::AABB, aabb_vs_aabb);
    np.registerFunc(ShapeType::Sphere, ShapeType::Sphere, sphere_vs_sphere);
    np.registerFunc(ShapeType::AABB, ShapeType::Sphere, aabb_vs_sphere);
    np.registerFunc(ShapeType::OBB, ShapeType::OBB, obb_vs_obb);
    np.registerFunc(ShapeType::Capsule, ShapeType::Capsule, capsule_vs_capsule);
    np.registerFunc(ShapeType::Sphere, ShapeType::Plane, sphere_vs_plane);
    np.registerFunc(ShapeType::AABB, ShapeType::Plane, aabb_vs_plane);
    np.registerFunc(ShapeType::Sphere, ShapeType::Triangle, sphere_vs_triangle);
    np.registerFunc(ShapeType::ConvexPolyhedron, ShapeType::ConvexPolyhedron, convex_vs_convex);

    Shape s1{ShapeType::AABB, AABB{{0,0,0},{2,2,2}}};
    Shape s2{ShapeType::AABB, AABB{{1,1,1},{3,3,3}}};
    Shape s3{ShapeType::Sphere, Sphere{{1,1,1}, 1.0f}};
    Shape s4{ShapeType::Sphere, Sphere{{5,5,5}, 1.0f}};
    Shape s5{ShapeType::OBB, OBB{{0,0,0},{1,1,1}, {{{1,0,0},{0,1,0},{0,0,1}}}}};
    Shape s6{ShapeType::OBB, OBB{{1,0,0},{1,1,1}, {{{1,0,0},{0,1,0},{0,0,1}}}}};
    Shape s7{ShapeType::Capsule, Capsule{{0,0,0},{1,0,0},0.5f}};
    Shape s8{ShapeType::Capsule, Capsule{{0.5f,0,0},{1.5f,0,0},0.5f}};
    Shape s9{ShapeType::Plane, Plane{{0,1,0},-1}};
    Shape s10{ShapeType::Triangle, Triangle{{0,0,0},{1,0,0},{0,1,0}}};
    Shape s11{ShapeType::ConvexPolyhedron, ConvexPolyhedron{{{0,0,0},{1,0,0},{0,1,0},{0,0,1}},{{0,1,2},{0,1,3},{0,2,3},{1,2,3}}}};
    Shape s12{ShapeType::ConvexPolyhedron, ConvexPolyhedron{{{0.5f,0.5f,0.5f},{1.5f,0.5f,0.5f},{0.5f,1.5f,0.5f},{0.5f,0.5f,1.5f}},{{0,1,2},{0,1,3},{0,2,3},{1,2,3}}}};
    Shape s13{ShapeType::Plane, Plane{{0,1,0},-5}};
    Shape s14{ShapeType::Sphere, Sphere{{0.25f,0.25f,0.2f},0.25f}};

    CHECK(np.test(s1, s2), "overlapping AABB pair");
    CHECK(np.test(s1, s3), "AABB versus sphere pair");
    CHECK(np.test(s3, s1), "reverse AABB versus sphere dispatch");
    CHECK(!np.test(s3, s4), "separated sphere pair");
    CHECK(np.test(s5, s6), "overlapping OBB pair");
    CHECK(np.test(s7, s8), "overlapping capsule pair");
    CHECK(np.test(s3, s9), "sphere touching plane");
    CHECK(np.test(s1, s9), "AABB crossing plane");
    CHECK(np.test(s14, s10), "sphere contained by triangle projection");
    CHECK(np.test(s11, s12), "overlapping convex-polyhedron pair");
    CHECK(!np.test(s9, s13), "missing plane versus plane registration");

    std::cout << "Narrowphase assertions passed.\n";
    return 0;
}
