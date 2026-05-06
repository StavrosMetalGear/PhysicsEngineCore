#include "physics/ConvexPolyhedron.h"
#include <iostream>

int main() {
    // Simple tetrahedron
    ConvexPolyhedron a{{
        {0,0,0}, {1,0,0}, {0,1,0}, {0,0,1}
    }, {
        {0,1,2}, {0,1,3}, {0,2,3}, {1,2,3}
    }};
    // Shifted tetrahedron
    ConvexPolyhedron b{{
        {0.5f,0.5f,0.5f}, {1.5f,0.5f,0.5f}, {0.5f,1.5f,0.5f}, {0.5f,0.5f,1.5f}
    }, {
        {0,1,2}, {0,1,3}, {0,2,3}, {1,2,3}
    }};
    // Far tetrahedron
    ConvexPolyhedron c{{
        {5,5,5}, {6,5,5}, {5,6,5}, {5,5,6}
    }, {
        {0,1,2}, {0,1,3}, {0,2,3}, {1,2,3}
    }};
    std::cout << "a intersects b: " << intersects_convex_polyhedron(a, b) << std::endl;
    std::cout << "a intersects c: " << intersects_convex_polyhedron(a, c) << std::endl;
    return 0;
}
