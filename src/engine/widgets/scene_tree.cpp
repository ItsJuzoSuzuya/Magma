#include "scene_tree.hpp"
#include "../scene.hpp"
#include "imgui.h"
#include "ui_context.hpp"

using namespace std;
namespace Magma {

// Pre-frame: Just attach Scene Tree
void SceneTree::preFrame() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin(name());
  ImGui::End();
}

// Draw: Simple tree node
void SceneTree::draw() {
  UIContext::ensureInit();

  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin(name());

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) 
    SceneMenu::queueContextMenuFor(nullptr);

  Scene::drawTree();
  sceneMenu.draw();

  ImGui::End();
}

} // namespace Magma
