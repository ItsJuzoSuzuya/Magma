#include "scene.hpp"

namespace Magma {

Scene::Scene() {
  if (activeScene == nullptr)
    activeScene = this;
}

void Scene::addGameObject(std::unique_ptr<GameObject> gameObject) {
  if (gameObject == nullptr)
    return;

  gameObjects.push_back(std::move(gameObject));
}

} // namespace Magma
