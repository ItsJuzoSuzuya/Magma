#include "scene_tree.hpp"
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
  ImGui::TreeNode("Triangle");
  ImGui::End();
}

} // namespace Magma
