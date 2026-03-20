module; 
#include <cstdint>
#include <imgui.h>
#include <glm/vec4.hpp>

module components:point_light;

namespace Magma {

export struct PointLightData {
  glm::vec4 position;
  glm::vec4 color;
};

export struct PointLightSSBO {
  uint32_t lightCount;
  alignas(16) PointLightData lights[128];
};

export class PointLight: public Component{
public:
  PointLight(GameObject *owner) : Component(owner) {}

  void onRender(SceneRenderer &renderer) {
    // Keep light position in sync with the owner's Transform
    if (owner) {
      if (auto *t = owner->getComponent<Transform>())
        lightData.position = glm::vec4(t->position, 1.0f);
    }
    renderer.submitPointLight(lightData);
  }

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

private:
  PointLightData lightData{glm::vec4{0.f, 0.f, 0.f, 1.f},
                           glm::vec4{1.f, 1.f, 1.f, 1.f}};

};

} // namespace Magma
