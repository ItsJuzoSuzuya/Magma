#include "collision.hpp"
#include <algorithm>
#include <glm/fwd.hpp>

using namespace std;

namespace magma {

BoxCollider::BoxCollider(Transform &transform)
    : position(transform.position), rotation(transform.rotation),
      scale(transform.scale) {
  collisionBox.min = position - scale;
  collisionBox.max = position + scale;
}

BoxCollider::BoxCollider(const glm::vec3 &min, const glm::vec3 &max)
    : position(min + max / 2.0f), rotation(0, 0, 0), scale(1, 1, 1) {
  collisionBox.min = min;
  collisionBox.max = max;
}

bool BoxCollider::checkCollision(const CollisionBox3D &other) {
  return (collisionBox.min.x <= other.max.x &&
          collisionBox.max.x >= other.min.x) &&
         (collisionBox.min.y <= other.max.y &&
          collisionBox.max.y >= other.min.y) &&
         (collisionBox.min.z <= other.max.z &&
          collisionBox.max.z >= other.min.z);
}

Collision3D BoxCollider::resolveCollision(const CollisionBox3D &other) {
  Collision3D collision;
  collision.isColliding = false;

  if (checkCollision(other)) {
    collision.isColliding = true;

    float xOverlap = std::min(collisionBox.max.x, other.max.x) -
                     std::max(collisionBox.min.x, other.min.x);
    float yOverlap = std::min(collisionBox.max.y, other.max.y) -
                     std::max(collisionBox.min.y, other.min.y);
    float zOverlap = std::min(collisionBox.max.z, other.max.z) -
                     std::max(collisionBox.min.z, other.min.z);

    if (xOverlap < yOverlap && xOverlap < zOverlap) {
      collision.direction = {1, 0, 0};
      collision.length = xOverlap;
    } else if (yOverlap < xOverlap && yOverlap < zOverlap) {
      collision.direction = {0, 1, 0};
      collision.length = yOverlap;
    } else if (zOverlap < xOverlap && zOverlap < yOverlap) {
      collision.direction = {0, 0, 1};
      collision.length = zOverlap;
    }

    if (collisionBox.min.x < other.min.x)
      collision.direction.x *= -1;
    if (collisionBox.min.y < other.min.y)
      collision.direction.y *= -1;
    if (collisionBox.min.z < other.min.z)
      collision.direction.z *= -1;
  }
  return collision;
}

} // namespace magma
