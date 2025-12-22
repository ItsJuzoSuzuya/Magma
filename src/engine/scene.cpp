#include "scene.hpp"
#include "core/device.hpp"
#include "core/renderer.hpp"
#include "components/transform.hpp"
#include "gameobject.hpp"
#include "scene_action.hpp"
#include <memory>

#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
  #include "widgets/inspector.hpp"
  #include "widgets/scene_menu.hpp"
#endif

namespace Magma {

Scene::Scene() {
  if (activeScene == nullptr)
    setActive();

  auto &camera = GameObject::create("Main Camera");
  camera.addComponent<Transform>();
  Camera *camComp = camera.addComponent<Camera>();
  camComp->setPerspectiveProjection(glm::radians(60.0f), 16.0f / 9.0f, 0.1f,
                                    100.0f);
  Scene::setActiveCamera(camComp);
}

Scene::~Scene() {
  Device::waitIdle();

  if (activeScene == this)
    activeScene = nullptr;
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

// GameObject management 
std::vector<std::unique_ptr<GameObject>> &Scene::getGameObjects() {
  return gameObjects;
}

GameObject *Scene::findGameObjectById(GameObject::id_t id) {
  if (activeScene == nullptr)
    return nullptr;

  std::function<GameObject *(GameObject *)> findObjectInChildren =
      [&](GameObject *node) -> GameObject * {
    if (!node)
      return nullptr;

    if (node->id == id)
      return node;

    auto children = node->getChildren();
    for (auto *child : children) {
      if (auto obj = findObjectInChildren(child))
        return obj;
    }
    return nullptr;
  };

  for (const auto &go : activeScene->gameObjects) {
    if (!go) 
      continue;

    if (go->id == id) 
      return go.get();

    if (auto obj = findObjectInChildren(go.get())) 
      return obj;
  }
  return nullptr;
}

GameObject &Scene::addGameObject(std::unique_ptr<GameObject> gameObject) {
  assert(gameObject != nullptr &&
         "GameObject cannot be null when adding to scene");

  GameObject &ref = *gameObject;
  gameObjects.push_back(std::move(gameObject));
  return ref;
}

void Scene::removeGameObject(GameObject *gameObject) {
  defer(SceneAction::remove(gameObject));
}

// Scene Tree (Editor Only)
#if defined(MAGMA_WITH_EDITOR)
void Scene::drawSceneTree() {
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
#endif

// Rendering
void Scene::onRender(Renderer &renderer) {
  if (activeScene == nullptr)
    return;

  for (auto &gameObject : activeScene->gameObjects) {
    if (gameObject)
      gameObject->onRender(renderer);
  }
}

// Deferred Actions
void Scene::processDeferredActions() {
  if (activeScene == nullptr)
    return;
  if (deferredActions.empty())
    return;

  Device::waitIdle();
  for (auto &action : deferredActions)
    action();

  deferredActions.clear();
}

} // namespace Magma
