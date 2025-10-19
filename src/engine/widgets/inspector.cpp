#include "inspector.hpp"
#include "../gameobject.hpp"
#include "imgui.h"
#include "inspector_menu.hpp"

using namespace std;
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

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) {
    MenuContext context;
    context.target = contextTarget;
    context.device = device;
    InspectorMenu::queueContextMenuFor(context);
  }

  if (contextTarget) {
    for (auto *component : contextTarget->getComponents())
      component->onInspector();
  }

  inspectorMenu.draw();

  ImGui::End();
}

} // namespace Magma
