#include "scene_menu.hpp"
#include "../scene.hpp"
#include "../scene_action.hpp"
#include "../components/camera.hpp"
#include "../components/transform.hpp"
#include "../gameobject.hpp"
#include "engine/scene_manager.hpp"
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
        SceneManager::activeScene->defer(SceneAction::remove(target));
    } else {
      ImGui::TextUnformatted("Scene");
      ImGui::Separator();
      if (ImGui::MenuItem("Add Entity")) {
        if (Scene *scene = SceneManager::activeScene) 
          scene->createGameObject();
      }
      if (ImGui::MenuItem("Add Camera")) {
        if (Scene *scene = SceneManager::activeScene) {
          auto obj = std::make_unique<GameObject>("Camera");
          obj->addComponent<Transform>();
          obj->addComponent<Camera>();
          scene->addGameObject(std::move(obj));
        }
      }
    }

    ImGui::EndPopup();
  }
}

} // namespace Magma
