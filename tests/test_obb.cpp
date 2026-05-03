#include "physics/OBB.h"
#include <iostream>
#include <array>

// Helper to create identity axes
std::array<std::array<float, 3>, 3> identity_axes() {
    return {{{1,0,0},{0,1,0},{0,0,1}}};
}

int main() {
    OBB obb1{{0,0,0}, {1,1,1}, identity_axes()};
    OBB obb2{{1.5f,0,0}, {1,1,1}, identity_axes()};
    OBB obb3{{3,0,0}, {1,1,1}, identity_axes()};
    std::cout << "obb1 intersects obb2: " << obb1.intersects(obb2) << std::endl;
    std::cout << "obb1 intersects obb3: " << obb1.intersects(obb3) << std::endl;
    return 0;
}
