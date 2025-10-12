#include "scene.hpp"
#include "imgui.h"
#include "widgets/inspector.hpp"
#include "widgets/scene_menu.hpp"
#include <algorithm>
#include <memory>
#include <print>

using namespace std;
namespace Magma {

Scene::Scene() {
  if (activeScene == nullptr)
    setActive();
}

// Destructor
Scene::~Scene() {
  if (activeScene == this)
    activeScene = nullptr;
}

void Scene::removeGameObject(GameObject *gameObject) {
  if (gameObject == nullptr)
    return;

  auto &gameObjects = activeScene->gameObjects;
  auto it = find_if(gameObjects.begin(), gameObjects.end(),
                    [gameObject](const unique_ptr<GameObject> &obj) {
                      return obj.get() == gameObject;
                    });
  if (it != gameObjects.end()) {
    println("Removing GameObject: {}", gameObject->name);
    gameObjects.erase(it);
    Inspector::setContext(nullptr);
    println("GameObject removed successfully.");
  }
}

// --- Public ---
// Add empty GameObject
void Scene::addGameObject(unique_ptr<GameObject> gameObject) {
  if (gameObject == nullptr)
    return;

  gameObjects.push_back(std::move(gameObject));
}

// Draw all GameObjects recursively
void Scene::drawTree() {
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
      if (ImGui::IsItemClicked())
        Inspector::setContext(gameObject.get());

      continue;
    }

    // Else display as tree node
    bool open = ImGui::TreeNodeEx(gameObject->name.c_str(), flags);

    if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
      SceneMenu::queueContextMenuFor(gameObject.get());
    if (ImGui::IsItemClicked())
      Inspector::setContext(gameObject.get());

    if (open) {
      gameObject->drawChildren();
      ImGui::TreePop();
    }
  }
}

// Render all GameObjects
void Scene::onRender(Renderer &renderer) {
  if (activeScene == nullptr)
    return;

  for (auto &gameObject : activeScene->gameObjects) {
    if (gameObject)
      gameObject->onRender(renderer);
  }
}

} // namespace Magma
