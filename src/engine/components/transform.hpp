#pragma once

#include "component.hpp"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <istream>
namespace Magma {

class Transform : public Component {
public:
  Transform() = default;
  ~Transform() = default;

  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 rotation{0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

  // Lifecycle
  void onRender(Renderer &renderer) override;
  void onAwake() override {};
  void onUpdate() override {};

private:
  glm::mat4x4 mat4() const;
  glm::mat4 normalMatrix() const;
};

} // namespace Magma
