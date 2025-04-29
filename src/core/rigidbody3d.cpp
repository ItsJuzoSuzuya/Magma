#include "rigidbody3d.hpp"
#include <glm/ext/vector_float3.hpp>

namespace magma {

void RigidBody3D::applyGravity(const float dt) { velocity.y -= gravity * dt; }

void RigidBody3D::applyForce(const glm::vec3 &force) { velocity += force; }

void RigidBody3D::update(GameObject &gameObject, const float deltaTime) {
  return; // TODO: Implement this function
}

void RigidBody3D::resetVelocity() { velocity = glm::vec3{0.f}; }

} // namespace magma
