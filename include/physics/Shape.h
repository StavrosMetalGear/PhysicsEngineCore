#pragma once
#include <variant>
#include "physics/AABB.h"
#include "physics/Sphere.h"
#include "physics/OBB.h"
#include "physics/Capsule.h"
#include "physics/Plane.h"
#include "physics/Triangle.h"
#include "physics/ConvexPolyhedron.h"

enum class ShapeType {
    AABB,
    Sphere,
    OBB,
    Capsule,
    Plane,
    Triangle,
    ConvexPolyhedron
};

using ShapeData = std::variant<AABB, Sphere, OBB, Capsule, Plane, Triangle, ConvexPolyhedron>;

struct Shape {
    ShapeType type;
    ShapeData data;
};
