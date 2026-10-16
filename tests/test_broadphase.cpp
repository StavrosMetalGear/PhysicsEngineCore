#include "physics/Broadphase.h"
#include <iostream>
#include <set>

static std::set<std::pair<int, int>> normalizedPairs(const std::vector<std::pair<int, int>>& pairs)
{
    std::set<std::pair<int, int>> result;
    for (const auto& pair : pairs) {
        result.emplace(std::min(pair.first, pair.second), std::max(pair.first, pair.second));
    }
    return result;
}

int main() {
    Broadphase empty;
    if (!empty.computePairs().empty()) {
        std::cerr << "FAIL: empty broadphase should return no pairs\n";
        return 1;
    }

    Broadphase broadphase;
    broadphase.addObject(1, {{-5,-5,-5}, {-3,-3,-3}});
    broadphase.addObject(2, {{-4,-4,-4}, {-2,-2,-2}});
    broadphase.addObject(3, {{-3,-3,-3}, {-1,-1,-1}});
    broadphase.addObject(4, {{0,0,0}, {1,1,1}});
    broadphase.addObject(5, {{1,1,1}, {2,2,2}});

    const auto pairs = normalizedPairs(broadphase.computePairs());
    const std::set<std::pair<int, int>> expected{{1, 2}, {1, 3}, {2, 3}, {4, 5}};
    if (pairs != expected) {
        std::cerr << "FAIL: unexpected broadphase pair set\n";
        return 1;
    }

    std::cout << "Broadphase assertions passed.\n";
    return 0;
}
