#include "scene.hpp"
#include "gameobject.hpp"
#include "imgui.h"
#include "scene_action.hpp"
#include "widgets/inspector.hpp"
#include "widgets/scene_menu.hpp"
#include <memory>

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

// --- Public --- //
// --- GameObject management ---
GameObject &Scene::addGameObject(unique_ptr<GameObject> gameObject) {
  assert(gameObject != nullptr &&
         "GameObject cannot be null when adding to scene");

  GameObject &ref = *gameObject;
  gameObjects.push_back(std::move(gameObject));
  return ref;
}

void Scene::removeGameObject(GameObject *gameObject) {
  defer(SceneAction::remove(gameObject));
}

// --- Scene operations ---
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

void Scene::onRender(Renderer &renderer) {
  if (activeScene == nullptr)
    return;

  for (auto &gameObject : activeScene->gameObjects) {
    if (gameObject)
      gameObject->onRender(renderer);
  }
}

void Scene::processDeferredActions() {
  if (activeScene == nullptr)
    return;

  for (auto &action : deferredActions)
    action();

  deferredActions.clear();
}
} // namespace Magma
