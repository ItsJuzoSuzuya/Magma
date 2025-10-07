#include "scene_tree.hpp"
#include "../scene.hpp"
#include "imgui.h"

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
  ImGui::End();
}

} // namespace Magma
