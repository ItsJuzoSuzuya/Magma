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
    if (auto *target = contextState.target) {
      ImGui::TextUnformatted(target->name.c_str());
      ImGui::Separator();
      drawAddComponentMenu();
    } else {
      ImGui::TextUnformatted("Scene");
      ImGui::Separator();
      if (ImGui::MenuItem("Add Entity"))
        GameObject::create();
    }

    ImGui::EndPopup();
  }
}

void InspectorMenu::drawAddComponentMenu() {
  if (!contextState.target && !contextState.device)
    return;

  if (ImGui::BeginMenu("Add Component")) {
    if (ImGui::MenuItem("Transform"))
      contextState.target->addComponent<Transform>();
    if (ImGui::MenuItem("Mesh"))
      contextState.target->addComponent<Mesh>(*contextState.device);
    ImGui::EndMenu();
  }
}

} // namespace Magma
