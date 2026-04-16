#pragma once
#include "component.hpp"
#include <glm/vec4.hpp>

namespace Magma {

struct PointLightData {
  glm::vec4 position;
  glm::vec4 color;
};

struct PointLightSSBO {
  uint32_t lightCount;
  alignas(16) PointLightData lights[128];
};

class PointLight : public Component {
public:
  PointLight() : Component() {}

  void onUpdate() override {}
  void collectProxy(RenderProxy &proxy) override;

  #if defined(MAGMA_WITH_EDITOR)
    void onInspector() override;
    const char *inspectorName() const override { return "Point Light"; }
    const float inspectorHeight() const override { return 100.f; }
  #endif

  PointLightData lightData{glm::vec4{0.f, 0.f, 0.f, 1.f}, glm::vec4{1.f, 1.f, 1.f, 1.f}};
};

} // namespace Magma
