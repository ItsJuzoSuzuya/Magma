#include "point_light.hpp"
#include "engine/gameobject.hpp"
#include "engine/render/scene_renderer.hpp"
#include "imgui.h"
#include "transform.hpp"

namespace Magma {

PointLight::PointLight(GameObject *owner) : Component(owner) {}

void PointLight::onRender(SceneRenderer &renderer) {
  // Keep light position in sync with the owner's Transform
  if (owner) {
    if (auto *t = owner->getComponent<Transform>())
      lightData.position = glm::vec4(t->position, 1.0f);
  }
  renderer.submitPointLight(lightData);
}

#if defined(MAGMA_WITH_EDITOR)
void PointLight::onInspector() {
  // Draw simple controls (no Begin/End here; Inspector wraps us)
  ImGui::TextDisabled("Point Light");
  ImGui::ColorEdit3("Color", &lightData.color.x);
  ImGui::DragFloat("Intensity", &lightData.color.w, 0.01f, 0.0f, 100.0f);
}
#endif

} // namespace Magma
