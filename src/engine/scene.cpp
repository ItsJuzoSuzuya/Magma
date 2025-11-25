#include "scene.hpp"
#include "../core/renderer.hpp"
#include "../core/device.hpp"
#include "components/transform.hpp"
#include "gameobject.hpp"
#include "scene_action.hpp"

#if defined(MAGMA_WITH_EDITOR)
#include "imgui.h"
#include "widgets/inspector.hpp"
#include "widgets/scene_menu.hpp"
#endif

#include <memory>

using namespace std;
namespace Magma {

Scene::Scene() {
  if (activeScene == nullptr)
    setActive();
  
  auto &camera = GameObject::create("Main Camera");
  camera.addComponent<Transform>();
  camera.addComponent<Camera>();
  Camera *camComp = camera.getComponent<Camera>();
  camComp->setPerspectiveProjection(glm::radians(60.0f), 16.0f/9.0f, 0.1f, 100.0f);
  Scene::setActiveCamera(camComp);
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
#if defined(MAGMA_WITH_EDITOR)
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
#endif

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
  if (deferredActions.empty())
    return;

  Device::waitIdle();
  for (auto &action : deferredActions)
    action();

  deferredActions.clear();
}

} // namespace Magma
