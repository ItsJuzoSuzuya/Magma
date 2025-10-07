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

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) 
    SceneMenu::queueContextMenuFor(nullptr);

  Scene::draw();
  sceneMenu.draw();

  ImGui::End();
}

} // namespace Magma
