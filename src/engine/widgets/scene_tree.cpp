#include "scene_tree.hpp"
#include "../gameobject.hpp"
#include "engine/scene_manager.hpp"
#include "imgui.h"
#include "inspector.hpp"
#include "ui_context.hpp"

using namespace std;
namespace Magma {

static void drawGameObjectNode(GameObject *obj) {
  if (!obj) return;

  ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen |
                             ImGuiTreeNodeFlags_SpanFullWidth;

  if (!obj->hasChildren()) {
    flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
    ImGui::TreeNodeEx(obj->name.c_str(), flags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
      SceneMenu::queueContextMenuFor(obj);
    if (ImGui::IsItemClicked())
      Inspector::setContext(obj);
    return;
  }

  bool open = ImGui::TreeNodeEx(obj->name.c_str(), flags);
  if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
    SceneMenu::queueContextMenuFor(obj);
  if (ImGui::IsItemClicked())
    Inspector::setContext(obj);

  if (open) {
    obj->drawChildren();
    ImGui::TreePop();
  }
}

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

  if (SceneManager::activeScene) {
    for (const auto &go : SceneManager::activeScene->getGameObjects()) {
      if (go) drawGameObjectNode(go.get());
    }
  }

  sceneMenu.draw();

  ImGui::End();
}

} // namespace Magma
