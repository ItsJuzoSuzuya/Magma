#include "point_light.hpp"
#include "../../core/renderer.hpp"
#include "../gameobject.hpp"
#include "imgui.h"
#include "transform.hpp"

namespace Magma {

PointLight::PointLight(GameObject *owner) : Component(owner) {}

void PointLight::onRender(Renderer &renderer) {
  // Keep light position in sync with the owner's Transform
  if (owner) {
    if (auto *t = owner->getComponent<Transform>())
      lightData.position = glm::vec4(t->position, 1.0f);
  }
  renderer.submitPointLight(lightData);
}

void PointLight::onInspector() {
#if defined(MAGMA_WITH_EDITOR)
  // Draw simple controls (no Begin/End here; Inspector wraps us)
  ImGui::TextDisabled("Point Light");
  ImGui::ColorEdit3("Color", &lightData.color.x);
  ImGui::DragFloat("Intensity", &lightData.color.w, 0.01f, 0.0f, 100.0f);
#endif
}

} // namespace Magma
