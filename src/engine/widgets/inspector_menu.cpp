#include "inspector_menu.hpp"
#include "../components/transform.hpp"
#include "../components/mesh.hpp"
#include "../gameobject.hpp"
#include "imgui.h"

using namespace std;
namespace Magma {

// Draw: Popup menu for scene
void InspectorMenu::draw() {
  if (openPopupRequested) {
    ImGui::OpenPopup(name());
    openPopupRequested = false;
  }

  // Popup menu
  if (ImGui::BeginPopup(name())) {
    if (contextTarget) {
      ImGui::TextUnformatted(contextTarget->name.c_str());
      ImGui::Separator();
      drawAddComponentMenu();
    } else {
      ImGui::TextUnformatted("Scene");
      ImGui::Separator();
      if (ImGui::MenuItem("Add Entity"))
        GameObject::create();
      if (ImGui::MenuItem("Delete"))
        ; // No scene deletion for now
    }

    ImGui::EndPopup();
  }
}

void InspectorMenu::drawAddComponentMenu() {
  if (!contextTarget)
    return;

  if (ImGui::BeginMenu("Add Component")) {
    if (ImGui::MenuItem("Transform"))
      contextTarget->addComponent<Transform>();
    if (ImGui::MenuItem("Mesh"))
      contextTarget->addComponent<Mesh>();
    ImGui::EndMenu();
  }
}

} // namespace Magma
