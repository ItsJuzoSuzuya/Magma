#include "scene.hpp"
#include "core/device.hpp"
#include "gameobject.hpp"
#include "scene_action.hpp"
#include <cassert>
#include <functional>
#include <memory>

namespace Magma {

GameObject *Scene::createGameObject(){
  auto go = std::make_unique<GameObject>();
  GameObject *goPtr = go.get();
  gameObjects.push_back(std::move(go));
  return goPtr;
}
GameObject *Scene::createGameObject(std::string name){
  auto go = std::make_unique<GameObject>(name);
  GameObject *goPtr = go.get();
  gameObjects.push_back(std::move(go));
  return goPtr;
}
GameObject *Scene::createGameObject(GameObject* parent){
  auto go = std::make_unique<GameObject>(parent);
  GameObject *goPtr = go.get();
  gameObjects.push_back(std::move(go));
  return goPtr;
}
GameObject *Scene::createGameObject(GameObject* parent, std::string name){
  auto go = std::make_unique<GameObject>(parent, name);
  GameObject *goPtr = go.get();
  gameObjects.push_back(std::move(go));
  return goPtr;
}

GameObject *Scene::addGameObject(std::unique_ptr<GameObject> gameObject) {
  assert(gameObject != nullptr && "GameObject cannot be null when adding to scene");
  GameObject *ref = gameObject.get();
  gameObjects.push_back(std::move(gameObject));
  return ref;
}
void Scene::removeGameObject(GameObject *gameObject) {
  defer(SceneAction::remove(gameObject));
}

void Scene::processDeferredActions() {
  if (deferredActions.empty())
    return;

  Device::waitIdle();
  for (auto &action : deferredActions)
    action();
  deferredActions.clear();
}

} // namespace Magma
