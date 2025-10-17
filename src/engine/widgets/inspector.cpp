#include "inspector.hpp"
#include "../gameobject.hpp"
#include "imgui.h"

namespace Magma {

// --- Public --- //
// --- Rendering & Drawing ---
bool Inspector::preFrame() {
  ImGui::Begin(name());
  ImGui::End();
  return true;
}

void Inspector::draw() {
  ImGui::Begin(name());
  if (context) {
    for (auto *component : context->getComponents())
      component->onInspector();
  }
  ImGui::End();
}

} // namespace Magma
