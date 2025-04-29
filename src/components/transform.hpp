
#include "component.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
namespace magma {

struct Transform : public Component {
  Transform(GameObject *parent) : Component(parent) {}
  Transform(GameObject *parent, glm::vec3 position)
      : Component(parent), position{position} {}
  Transform(GameObject *parent, float x, float y, float z)
      : Transform(parent, glm::vec3(x, y, z)) {}

  glm::vec3 position{};
  glm::vec3 rotation{};
  glm::vec3 scale{1.f, 1.f, 1.f};

  glm::mat4 mat4() const;
  glm::mat4 normalMatrix() const;
};

} // namespace magma
