#include "scene_tree.hpp"
#include "../scene.hpp"
#include "imgui.h"

using namespace std;
namespace Magma {

// Pre-frame: Just attach Scene Tree
bool SceneTree::preFrame() {
  ImGui::Begin(name());
  ImGui::End();
  return true;
}

// Draw: Simple tree node
void SceneTree::draw() {
  ImGui::Begin(name());
  Scene::draw();

  // Centralized popup for any GameObject (called once per frame)
  if (ImGui::BeginPopup("SceneMenu")) {
    if (auto *target = getContextTarget()) {
      ImGui::TextUnformatted(target->name.c_str());
      ImGui::Separator();
      if (ImGui::MenuItem("Add Child")) {
        target->addChild();
      }
      // if (ImGui::MenuItem("Rename...")) { /* start rename */ }
      // if (ImGui::MenuItem("Delete")) { /* delete target */ }
    }

    ImGui::EndPopup();
  }

  ImGui::End();
}

} // namespace Magma
