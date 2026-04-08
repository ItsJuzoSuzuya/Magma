module; 
#include <cstdint>
#include <imgui.h>
#include <glm/vec4.hpp>

export module components:point_light;
import :component;
import core;

namespace Magma {

export struct PointLightData {
  glm::vec4 position;
  glm::vec4 color;
};

export struct PointLightSSBO {
  uint32_t lightCount;
  alignas(16) PointLightData lights[128];
};

export class PointLight: public Component {
public:
  PointLight(uint32_t ownerID) : Component(ownerID) {}

  #if defined(MAGMA_WITH_EDITOR)
    void onInspector() {
      // Draw simple controls (no Begin/End here; Inspector wraps us)
      ImGui::TextDisabled("Point Light");
      ImGui::ColorEdit3("Color", &lightData.color.x);
      ImGui::DragFloat("Intensity", &lightData.color.w, 0.01f, 0.0f, 100.0f);
    }
    virtual const char *inspectorName() const override { return "Point Light"; }
    virtual const float inspectorHeight() const override { return 100.f; }
  #endif

  void collectProxy(RenderProxy &proxy) override {
    PointLightProxy plProxy = {};
    plProxy.position = lightData.position;
    plProxy.color = lightData.color;

    proxy.pointLight = plProxy;
  }

private:
  PointLightData lightData{glm::vec4{0.f, 0.f, 0.f, 1.f},
                           glm::vec4{1.f, 1.f, 1.f, 1.f}};

};

} // namespace Magma
