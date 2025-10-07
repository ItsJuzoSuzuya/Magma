#include "scene.hpp"
#include "imgui.h"
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
    if (gameObject != nullptr) {
      if (ImGui::TreeNodeEx(gameObject->name.c_str())) {
        gameObject->drawChildren();
        ImGui::TreePop();
      }
    }
  }
}

} // namespace Magma
