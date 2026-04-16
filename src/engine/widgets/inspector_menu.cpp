#include "inspector_menu.hpp"
#include "../components/mesh.hpp"
#include "../components/point_light.hpp"
#include "../components/transform.hpp"
#include "../gameobject.hpp"
#include "../scene.hpp"
#include "../scene_action.hpp"
#include "engine/scene_manager.hpp"
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
      ImGui::TextUnformatted(SceneManager::activeScene->getName().c_str());
      ImGui::Separator();
      if (ImGui::MenuItem("Add Entity")) {
        auto *scene = SceneManager::activeScene;
        if (scene) {
          scene->createGameObject();
        }
      }
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
    if (ImGui::MenuItem("Point Light"))
      contextTarget->addComponent<PointLight>();
    ImGui::EndMenu();
  }
}

} // namespace Magma
