#include "../collision.hpp"
#include <glm/ext/vector_float3.hpp>

namespace magma {

class RaycastHit {
public:
  bool isColliding = false;

  operator bool() const { return isColliding; }
};

class Raycast {
public:
  Raycast(const glm::vec3 &origin, const glm::vec3 &direction, float length);

  RaycastHit checkCollision(const BoxCollider &other);

  glm::vec3 origin;
  glm::vec3 direction;
  float length;
};

} // namespace magma
