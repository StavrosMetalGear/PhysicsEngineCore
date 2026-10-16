#pragma once
#include <cstddef>
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
    ConvexPolyhedron,
    Count
};

constexpr std::size_t ShapeTypeCount = static_cast<std::size_t>(ShapeType::Count);

using ShapeData = std::variant<AABB, Sphere, OBB, Capsule, Plane, Triangle, ConvexPolyhedron>;

struct Shape {
    ShapeType type;
    ShapeData data;
};
