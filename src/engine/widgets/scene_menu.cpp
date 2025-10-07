#include "scene_menu.hpp"
#include "../gameobject.hpp"
#include "imgui.h"

using namespace std;
namespace Magma {

// Draw: Popup menu for scene
void SceneMenu::draw() {
  if (openPopupRequested) {
    ImGui::OpenPopup(name());
    openPopupRequested = false;
  }

  // Popup menu
  if (ImGui::BeginPopup(name())) {
    if (auto *target = getContextTarget()) {
      ImGui::TextUnformatted(target->name.c_str());
      ImGui::Separator();
      if (ImGui::MenuItem("Add Child"))
        target->addChild();
      // if (ImGui::MenuItem("Rename...")) { /* start rename */ }
      // if (ImGui::MenuItem("Delete")) { /* delete target */ }
    } else {
      ImGui::TextUnformatted("Scene");
      ImGui::Separator();
      if (ImGui::MenuItem("Add Entity"))
        GameObject::create();
    }

    ImGui::EndPopup();
  }
}

} // namespace Magma
