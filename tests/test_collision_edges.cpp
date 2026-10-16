#include "physics/AABB.h"
#include "physics/Capsule.h"
#include "physics/ConvexPolyhedron.h"
#include "physics/OBB.h"
#include "physics/Plane.h"
#include "physics/Ray.h"
#include "physics/Sphere.h"
#include "physics/Triangle.h"

#include <cmath>
#include <iostream>

#define CHECK(condition, message) \
	do { \
		if (!(condition)) { \
			std::cerr << "FAIL: " << message << "\n"; \
			return 1; \
		} \
	} while (false)

int main()
{
	const AABB box{{-1.0f, -1.0f, -1.0f}, {1.0f, 1.0f, 1.0f}};
	const AABB touchingBox{{1.0f, -1.0f, -1.0f}, {2.0f, 1.0f, 1.0f}};
	const AABB negativeBox{{-2.0f, -2.0f, -2.0f}, {0.0f, 0.0f, 0.0f}};
	const AABB separatedBox{{2.1f, -1.0f, -1.0f}, {3.0f, 1.0f, 1.0f}};
	const AABB pointBox{{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 0.0f}};

	CHECK(box.intersects(touchingBox), "AABB touching boundary");
	CHECK(box.intersects(negativeBox), "AABB negative-coordinate overlap");
	CHECK(!box.intersects(separatedBox), "AABB separated on one axis");
	CHECK(box.intersects(pointBox), "degenerate point AABB inside box");

	const Sphere sphere{{0.0f, 0.0f, 0.0f}, 1.0f};
	const Sphere touchingSphere{{2.0f, 0.0f, 0.0f}, 1.0f};
	const Sphere pointSphere{{0.0f, 0.0f, 0.0f}, 0.0f};
	CHECK(sphere.intersects(touchingSphere), "sphere touching boundary");
	CHECK(sphere.intersects(pointSphere), "zero-radius sphere at center");
	CHECK(intersects_aabb_sphere(box, touchingSphere), "AABB/sphere touching boundary");

	const OBB obbA{{0,0,0}, {1,1,1}, {{{1,0,0},{0,1,0},{0,0,1}}}};
	const OBB obbTouching{{2,0,0}, {1,1,1}, {{{1,0,0},{0,1,0},{0,0,1}}}};
	const OBB obbSeparated{{3.1f,0,0}, {1,1,1}, {{{1,0,0},{0,1,0},{0,0,1}}}};
	CHECK(obbA.intersects(obbTouching), "OBB touching boundary");
	CHECK(!obbA.intersects(obbSeparated), "OBB separated on one axis");

	const Capsule capsule{{0,0,0}, {0,0,0}, 0.5f};
	const Sphere capsuleTouchingSphere{{1.0f,0,0}, 0.5f};
	CHECK(intersects_capsule_sphere(capsule, capsuleTouchingSphere) == Touching,
		  "degenerate capsule touching sphere");
	const Capsule secondCapsule{{1,0,0}, {1,0,0}, 0.5f};
	CHECK(intersects_capsule_capsule(capsule, secondCapsule),
		  "degenerate capsules touching");

	const Plane plane{{0,1,0}, -1.0f};
	CHECK(intersects_sphere_plane(Sphere{{0,2,0}, 1.0f}, plane),
		  "sphere touching plane");
	CHECK(!intersects_sphere_plane(Sphere{{0,3.1f,0}, 1.0f}, plane),
		  "sphere separated from plane");
	CHECK(intersects_aabb_plane(box, plane), "AABB crossing plane");

	const Triangle triangle{{0,0,0}, {1,0,0}, {0,1,0}};
	CHECK(point_in_triangle({0.25f,0.25f,0.0f}, triangle), "point inside triangle");
	CHECK(!point_in_triangle({0.75f,0.75f,0.0f}, triangle), "point outside triangle");
	const Triangle degenerateTriangle{{0,0,0}, {1,0,0}, {2,0,0}};
	CHECK(!point_in_triangle({0.5f,0.0f,0.0f}, degenerateTriangle),
		  "degenerate triangle rejected");
	CHECK(intersects_sphere_triangle(Sphere{{0.25f,0.25f,0.1f},0.1f}, triangle),
		  "sphere touching triangle plane inside projection");

	const Ray ray{{0,0,-3}, {0,0,1}};
	float t0 = 0.0f;
	float t1 = 0.0f;
	CHECK(intersects_ray_sphere(ray, sphere, t0, t1), "ray/sphere hit");
	CHECK(std::abs(t0 - 2.0f) < 1.0e-6f && std::abs(t1 - 4.0f) < 1.0e-6f,
		  "ray/sphere distances");
	const Ray parallelRay{{0,0,2}, {1,0,0}};
	float planeT = 0.0f;
	CHECK(!intersects_ray_plane(parallelRay, plane, planeT), "parallel ray/plane miss");

	const ConvexPolyhedron tetrahedron{
		{{0,0,0}, {1,0,0}, {0,1,0}, {0,0,1}},
		{{0,1,2}, {0,1,3}, {0,2,3}, {1,2,3}}
	};
	const ConvexPolyhedron overlappingTetrahedron{
		{{0.5f,0.5f,0.5f}, {1.5f,0.5f,0.5f}, {0.5f,1.5f,0.5f}, {0.5f,0.5f,1.5f}},
		{{0,1,2}, {0,1,3}, {0,2,3}, {1,2,3}}
	};
	CHECK(intersects_convex_polyhedron(tetrahedron, overlappingTetrahedron),
		  "overlapping convex polyhedra");

	std::cout << "Collision edge assertions passed.\n";
	return 0;
}
