#include "point_light.hpp"

#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
#endif

namespace Magma {

void PointLight::collectProxy(RenderProxy &proxy) {
  PointLightProxy plProxy = {};
  plProxy.position = lightData.position;
  plProxy.color = lightData.color;
  proxy.pointLight = plProxy;
}

#if defined(MAGMA_WITH_EDITOR)
void PointLight::onInspector() {
  ImGui::TextDisabled("Point Light");
  ImGui::ColorEdit3("Color", &lightData.color.x);
  ImGui::DragFloat("Intensity", &lightData.color.w, 0.01f, 0.0f, 100.0f);
}
#endif

} // namespace Magma
