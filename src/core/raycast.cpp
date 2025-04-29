#include "raycast.hpp"

using namespace std;
namespace magma {

Raycast::Raycast(const glm::vec3 &origin, const glm::vec3 &rotation,
                 float length)
    : origin(origin), length(length) {
  glm::vec3 directionInWorld;
  directionInWorld.x = cos(rotation.x) * sin(rotation.y);
  directionInWorld.y = -sin(rotation.x);
  directionInWorld.z = cos(rotation.x) * cos(rotation.y);

  direction = glm::normalize(directionInWorld);
}

RaycastHit Raycast::checkCollision(const BoxCollider &other) {
  glm::vec3 tMin = (other.collisionBox.min - origin) / direction;
  glm::vec3 tMax = (other.collisionBox.max - origin) / direction;

  float tNear =
      std::max(std::max(std::min(tMin.x, tMax.x), std::min(tMin.y, tMax.y)),
               std::min(tMin.z, tMax.z));
  float tFar =
      std::min(std::min(std::max(tMin.x, tMax.x), std::max(tMin.y, tMax.y)),
               std::max(tMin.z, tMax.z));

  RaycastHit hit;

  if (tNear > tFar || tFar < 0.0f) {
    hit.isColliding = false;
    return hit;
  }

  hit.isColliding = true;
  return hit;
}
} // namespace magma
