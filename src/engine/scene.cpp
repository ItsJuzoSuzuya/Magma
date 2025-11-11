#include "scene.hpp"
#include "../core/renderer.hpp"
#include "../core/device.hpp"
#include "components/camera.hpp"
#include "components/transform.hpp"
#include "gameobject.hpp"
#include "imgui.h"
#include "scene_action.hpp"
#include "widgets/inspector.hpp"
#include "widgets/scene_menu.hpp"
#include <memory>

using namespace std;
namespace Magma {

Scene::Scene() {
  cameraTransform = make_unique<Transform>(nullptr);
  camera = make_unique<Camera>(cameraTransform.get());

  camera->setPerspectiveProjection(
      glm::radians(90.f), 16.f / 9.f, 0.1f, 100.f);

  if (activeScene == nullptr)
    setActive();
}

// Destructor
Scene::~Scene() {
  if (activeScene == this)
    activeScene = nullptr;
}

// --- Public --- //


GameObject *Scene::findGameObjectById(GameObject::id_t id){
  if (activeScene == nullptr)
    return nullptr;

  std::function<GameObject*(GameObject*)> findFrom = [&](GameObject* node)->GameObject*{
      if (!node) return nullptr;
      if (node->id == id) return node;
      auto children = node->getChildren();
      for (auto *c : children) {
        if (auto r = findFrom(c)) return r;
      }
      return nullptr;
    };

    for (const auto &g : activeScene->gameObjects) {
      if (!g) continue;
      if (g->id == id) return g.get();
      if (auto r = findFrom(g.get())) return r;
    }
    return nullptr;
}

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
  activeScene->camera->onUpdate();
  activeScene->camera->onRender(renderer);

  if (activeScene == nullptr)
    return;

  for (auto &gameObject : activeScene->gameObjects) {
    if (gameObject)
      gameObject->onRender(renderer);
  }
}

void Scene::processDeferredActions() {
  Device::waitIdle();
  if (activeScene == nullptr)
    return;

  for (auto &action : deferredActions)
    action();

  deferredActions.clear();
}

void Scene::moveCameraAlongRight(float speed) {
  cameraTransform->position +=
      cameraTransform->right() * speed;
}

void Scene::moveCameraAlongForward(float speed) {
  cameraTransform->position +=
      cameraTransform->forward() * speed;
}

void Scene::moveCameraAlongUp(float speed) {
  cameraTransform->position +=
      cameraTransform->up() * speed;
}

} // namespace Magma
