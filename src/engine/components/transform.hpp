#pragma once

#include "component.hpp"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
namespace Magma {

class Transform : public Component {
public:
  Transform(GameObject *obj) : Component(obj) {}
  ~Transform() = default;

  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 rotation{0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

  // Lifecycle
  void onRender(SceneRenderer &renderer) override;
  void onAwake() override {};
  void onUpdate() override {};

  #if defined(MAGMA_WITH_EDITOR)
  // Inspector
  void onInspector() override;
  const char *inspectorName() const override { return "Transform"; }
  const float inspectorHeight() const override { return 110.0f; }
  #endif

  // Direction vectors
  glm::vec3 forward() const;
  glm::vec3 right() const;
  glm::vec3 up() const;

private:
  glm::mat4x4 mat4() const;
  glm::mat4 normalMatrix() const;
};

} // namespace Magma
