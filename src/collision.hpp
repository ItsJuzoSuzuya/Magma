#pragma once
#include "components/component.hpp"
#include "components/transform.hpp"
#include <glm/ext/vector_float3.hpp>
#include <glm/glm.hpp>
#include <vector>

namespace magma {

struct CollisionBox3D {
  glm::vec3 min;
  glm::vec3 max;
  glm::vec3 center() const { return (min + max) / 2.0f; }
  std::vector<glm::vec3> corners() const {
    std::vector<glm::vec3> corners(8);
    corners.push_back(min);
    corners.push_back({min.x, min.y, max.z});
    corners.push_back({min.x, max.y, min.z});
    corners.push_back({min.x, max.y, max.z});
    corners.push_back({max.x, min.y, min.z});
    corners.push_back({max.x, min.y, max.z});
    corners.push_back({max.x, max.y, min.z});
    corners.push_back(max);
    return corners;
  }
};

class Collision3D {
public:
  bool isColliding;

  glm::vec3 direction;
  float length;
};

class BoxCollider : public Component {
public:
  BoxCollider(Transform &transform);
  BoxCollider(const glm::vec3 &min, const glm::vec3 &max);

  bool checkCollision(const CollisionBox3D &other);
  Collision3D resolveCollision(const CollisionBox3D &other);

  glm::vec3 position;
  glm::vec3 rotation;
  glm::vec3 scale;

  CollisionBox3D collisionBox;
};

} // namespace magma
