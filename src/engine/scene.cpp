#include "scene.hpp"
#include "../core/renderer.hpp"
#include "../core/render_system.hpp"
#include "../core/frame_info.hpp"
#include "camera.hpp"
#include "gameobject.hpp"
#include "imgui.h"
#include "scene_action.hpp"
#include "widgets/inspector.hpp"
#include "widgets/scene_menu.hpp"
#include <memory>
#include <print>

using namespace std;
namespace Magma {

Scene::Scene() {
  camera = make_unique<Camera>(); 
  camera->setPerspectiveProjection(
      glm::radians(90.f), 16.f / 9.f, 0.1f, 100.f);
  camera->setView({0.f, 0.f, -5.f}, {0.f, 0.f, 0.f});

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
  activeScene->camera->pushCameraDataToGPU(renderer.getCameraBuffer(
      FrameInfo::frameIndex));

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
} // namespace Magma
