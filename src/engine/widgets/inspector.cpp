#include "inspector.hpp"
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
  ImGui::TextUnformatted("Hello from the inspector!");
  ImGui::End();
}

} // namespace Magma
