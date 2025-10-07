#include "scene.hpp"
#include "imgui.h"
#include "widgets/scene_menu.hpp"
#include "widgets/scene_tree.hpp"
#include <memory>

using namespace std;
namespace Magma {

Scene::Scene() {
  if (activeScene == nullptr)
    setActive();
}

Scene::~Scene() {
  if (activeScene == this)
    activeScene = nullptr;
}

// --- Public ---
// Add empty GameObject
void Scene::addGameObject(unique_ptr<GameObject> gameObject) {
  if (gameObject == nullptr)
    return;

  gameObjects.push_back(std::move(gameObject));
}

// Draw all GameObjects recursively
void Scene::draw() {
  if (activeScene == nullptr)
    return;

  for (auto &gameObject : activeScene->gameObjects) {
    if (gameObject == nullptr)
      continue;

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;

    // If no children, display as leaf node
    if (!gameObject->hasChildren()) {
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
      ImGui::TreeNodeEx(gameObject->name.c_str(), flags);

      if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
        SceneMenu::queueContextMenuFor(gameObject.get());

      continue;
    }

    // Else display as tree node
    bool open = ImGui::TreeNodeEx(gameObject->name.c_str(), flags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
      SceneMenu::queueContextMenuFor(gameObject.get());

    if (open) {
      gameObject->drawChildren();
      ImGui::TreePop();
    }
  }
}

} // namespace Magma
