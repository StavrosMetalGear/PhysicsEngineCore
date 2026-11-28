#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace physics::experimental {

using BodyId = std::uint64_t;
constexpr BodyId invalidBodyId = 0;

struct SphereBody {
	std::array<float, 3> position;
	std::array<float, 3> velocity;
	float radius;
};

class SimulationWorld {
public:
	SimulationWorld(std::size_t capacity, float fixedStepSeconds, std::array<float, 3> gravity);

	BodyId addSphere(const SphereBody& body);
	bool getSphere(BodyId id, SphereBody& body) const;
	bool removeSphere(BodyId id);
	bool step();
	std::size_t bodyCount() const noexcept;

private:
	struct Entry {
		BodyId id;
		SphereBody body;
	};

	bool advance(const SphereBody& current, SphereBody& next) const;

	std::size_t capacity_;
	float fixedStepSeconds_;
	std::array<float, 3> gravity_;
	BodyId nextId_ = 1;
	std::vector<Entry> bodies_;
};

}
