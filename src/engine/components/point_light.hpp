#pragma once
#include "component.hpp"
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Magma {

struct PointLightData {
  glm::vec4 position;
  glm::vec4 color;
};

struct PointLightSSBO {
  PointLightData lights[128];
  uint32_t lightCount;
  alignas(16) glm::vec3 padding;
};

class PointLight : public Component {
public:
  PointLight(GameObject *owner);

  void onAwake() override {};
  void onUpdate() override {};
  void onRender(Renderer &renderer) override;

#if defined(MAGMA_WITH_EDITOR)
  void onInspector() override;
  virtual const char *inspectorName() const override { return "Point Light"; }
  virtual const float inspectorHeight() const override { return 100.f; }

#endif

private:
  PointLightData lightData{glm::vec4{0.f}, glm::vec4{1.f}};
};

} // namespace Magma
