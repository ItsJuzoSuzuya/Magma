#include "inspector.hpp"
#include "../gameobject.hpp"
#include "imgui.h"

namespace Magma {

// Pre-frame: Just attach Inspector
bool Inspector::preFrame() {
  ImGui::Begin(name());
  ImGui::End();
  return true;
}

// Draw: Simple text
void Inspector::draw() {
  ImGui::Begin(name());
  if (context) {
    for (auto *component : context->getComponents())
      component->onInspector();
  }
  ImGui::End();
}

} // namespace Magma
