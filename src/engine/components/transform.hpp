#pragma once
#include "component.hpp"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>

namespace Magma {

class Transform : public Component {
public:
  Transform(uint32_t ownerID) : Component(), ownerID{ownerID} {}
  ~Transform() = default;

  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 rotation{0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

  void onUpdate() override {}
  void collectProxy(RenderProxy &proxy) override;

  #if defined(MAGMA_WITH_EDITOR)
    void onInspector() override;
    const char *inspectorName() const override { return "Transform"; }
    const float inspectorHeight() const override { return 110.0f; }
  #endif

  // Direction vectors
  glm::vec3 forward() const;
  glm::vec3 right() const;
  glm::vec3 up() const;

  glm::mat4 modelMatrix() const;

private:
  glm::mat4 normalMatrix() const;

  uint32_t ownerID;
};

} // namespace Magma
