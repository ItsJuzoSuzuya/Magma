#include "game_object.hpp"
#include <glm/ext/vector_float3.hpp>

namespace magma {

class RigidBody3D {
public:
  float gravity = 10.f;

  glm::vec3 velocity{0.f};

  void applyGravity(const float deltaTime);
  void applyForce(const glm::vec3 &force);
  void update(GameObject &gameObject, const float deltaTime);
  void resetVelocity();
};
} // namespace magma
