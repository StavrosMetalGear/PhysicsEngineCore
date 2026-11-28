#include "SimulationWorld.h"

#include <cmath>
#include <limits>
#include <stdexcept>

namespace physics::experimental {
namespace {

bool finiteVector(const std::array<float, 3>& values)
{
	for (float value : values) {
		if (!std::isfinite(value)) return false;
	}
	return true;
}

bool validBody(const SphereBody& body)
{
	return finiteVector(body.position) && finiteVector(body.velocity) &&
		std::isfinite(body.radius) && body.radius > 0.0f;
}

}

SimulationWorld::SimulationWorld(std::size_t capacity, float fixedStepSeconds, std::array<float, 3> gravity)
	: capacity_(capacity), fixedStepSeconds_(fixedStepSeconds), gravity_(gravity)
{
	if (!std::isfinite(fixedStepSeconds_) || fixedStepSeconds_ <= 0.0f || !finiteVector(gravity_)) {
		throw std::invalid_argument("Invalid simulation timestep or gravity");
	}
	bodies_.reserve(capacity_);
}

BodyId SimulationWorld::addSphere(const SphereBody& body)
{
	if (!validBody(body) || bodies_.size() >= capacity_ || nextId_ == invalidBodyId) return invalidBodyId;
	const BodyId id = nextId_;
	bodies_.push_back({id, body});
	nextId_ = id == std::numeric_limits<BodyId>::max() ? invalidBodyId : id + 1;
	return id;
}

bool SimulationWorld::getSphere(BodyId id, SphereBody& body) const
{
	for (const Entry& entry : bodies_) {
		if (entry.id == id) {
			body = entry.body;
			return true;
		}
	}
	return false;
}

bool SimulationWorld::removeSphere(BodyId id)
{
	for (auto entry = bodies_.begin(); entry != bodies_.end(); ++entry) {
		if (entry->id == id) {
			bodies_.erase(entry);
			return true;
		}
	}
	return false;
}

bool SimulationWorld::advance(const SphereBody& current, SphereBody& next) const
{
	next = current;
	for (std::size_t axis = 0; axis < 3; ++axis) {
		next.velocity[axis] += gravity_[axis] * fixedStepSeconds_;
		next.position[axis] += next.velocity[axis] * fixedStepSeconds_;
	}
	if (!finiteVector(next.position) || !finiteVector(next.velocity)) return false;
	if (next.position[1] < next.radius) {
		next.position[1] = next.radius;
		if (next.velocity[1] < 0.0f) next.velocity[1] = 0.0f;
	}
	return true;
}

bool SimulationWorld::step()
{
	SphereBody next{};
	for (const Entry& entry : bodies_) {
		if (!advance(entry.body, next)) return false;
	}
	for (Entry& entry : bodies_) {
		advance(entry.body, next);
		entry.body = next;
	}
	return true;
}

std::size_t SimulationWorld::bodyCount() const noexcept
{
	return bodies_.size();
}

}
