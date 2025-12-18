#include "scene_tree.hpp"
#include "../scene.hpp"
#include "imgui.h"
#include "ui_context.hpp"

using namespace std;
namespace Magma {

void SceneTree::preFrame() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin(name());
  ImGui::End();
}

void SceneTree::draw() {
  UIContext::ensureInit();

  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin(name());

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) 
    SceneMenu::queueContextMenuFor(nullptr);

  Scene::drawSceneTree();
  sceneMenu.draw();

  ImGui::End();
}

} // namespace Magma
