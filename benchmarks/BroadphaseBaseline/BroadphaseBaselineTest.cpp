#include "pch.h"
#include "CppUnitTest.h"
#include <limits>
#include "physics/Broadphase.h"
#include <chrono>
#include <string>
using namespace Microsoft::VisualStudio::CppUnitTestFramework;

namespace PhysicsEnginePerformance
{
    TEST_CLASS(BroadphaseBaselineTest)
    {
    public:
        TEST_METHOD(ComputeAdjacentPairs)
        {
            constexpr int objectCount = 512;
            constexpr int iterations = 10000;
            Broadphase broadphase;
            for (int i = 0; i < objectCount; ++i) {
                const float x = static_cast<float>(i);
                broadphase.addObject(i, {{x, 0.0f, 0.0f}, {x + 1.0f, 1.0f, 1.0f}});
            }
            const auto pairs = broadphase.computePairs();
            Assert::AreEqual(static_cast<size_t>(objectCount - 1), pairs.size());
            for (int i = 0; i < objectCount - 1; ++i) {
                Assert::AreEqual(i, pairs[i].first);
                Assert::AreEqual(i + 1, pairs[i].second);
            }
            for (int i = 0; i < 100; ++i) {
                broadphase.computePairs();
            }
            size_t totalPairs = 0;
            const auto start = std::chrono::steady_clock::now();
            for (int i = 0; i < iterations; ++i) {
                totalPairs += broadphase.computePairs().size();
            }
            const auto elapsed = std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start).count();
            Assert::AreEqual(static_cast<size_t>(objectCount - 1) * iterations, totalPairs);
            const std::wstring message = L"ComputeAdjacentPairs total ns: " + std::to_wstring(elapsed);
            Logger::WriteMessage(message.c_str());
        }
    };
}
