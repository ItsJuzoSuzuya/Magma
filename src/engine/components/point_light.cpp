#include "point_light.hpp"
#include "../../core/renderer.hpp"
#include "imgui.h"

namespace Magma {

PointLight::PointLight(GameObject *owner) : Component(owner) {}

void PointLight::onRender(Renderer &renderer) {
  renderer.submitPointLight(lightData);
}

void PointLight::onInspector() {
  ImGui::Begin(inspectorName());
  ImGui::End();
}

} // namespace Magma
