#pragma once
#include <vector>
#include <utility>
#include <functional>
#include <iostream>
#include "physics/Shape.h"

class Narrowphase {
public:
    using ContactResult = bool; // For now, just intersection (can extend to contact info)

    // Register a collision function for a shape pair
    using CollisionFunc = std::function<ContactResult(const Shape&, const Shape&)>;
    void registerFunc(ShapeType a, ShapeType b, CollisionFunc func) {
        table[static_cast<int>(a)][static_cast<int>(b)] = func;
    }

    // Test a pair
    ContactResult test(const Shape& a, const Shape& b) const {
        auto f = table[static_cast<int>(a.type)][static_cast<int>(b.type)];
        if (f) return f(a, b);
        // Try symmetric
        f = table[static_cast<int>(b.type)][static_cast<int>(a.type)];
        if (f) return f(b, a);
        std::cerr << "No collision function for pair!\n";
        return false;
    }

private:
    CollisionFunc table[7][7] = {};
};
