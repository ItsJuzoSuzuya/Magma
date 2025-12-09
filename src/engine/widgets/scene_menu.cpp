#include "scene_menu.hpp"
#include "../scene_action.hpp"
#include "../components/camera.hpp"
#include "../components/transform.hpp"
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
      if (ImGui::MenuItem("Delete"))
        target->destroy();
    } else {
      ImGui::TextUnformatted("Scene");
      ImGui::Separator();
      if (ImGui::MenuItem("Add Entity"))
        GameObject::create();
      if (ImGui::MenuItem("Add Camera")) {
        auto &obj = GameObject::create("Camera");
        obj.addComponent<Transform>();
        obj.addComponent<Camera>();
      }
    }

    ImGui::EndPopup();
  }
}

} // namespace Magma
