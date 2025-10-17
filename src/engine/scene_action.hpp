#include "components/mesh.hpp"
#include "gameobject.hpp"
#include "scene.hpp"
#include <algorithm>
#include <functional>
namespace Magma {

class SceneAction {
public:
  inline static std::function<void()> remove(GameObject *obj) {
    return [obj]() {
      if (!obj)
        return;

      if (auto parent = obj->parent) {
        auto siblings = parent->getChildren();
        siblings.erase(
            std::remove_if(siblings.begin(), siblings.end(),
                           [&](const auto &child) { return child == obj; }),
            siblings.end());
      } else if (auto scene = Scene::current()) {
        auto &gameObjects = scene->getGameObjects();
        gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                           [&](const auto &go) { return go.get() == obj; }),
            gameObjects.end());
      }
    };
  }

  inline static std::function<void()> loadMesh(GameObject *obj) {
    return [obj]() {
      if (!obj)
        return;

      obj->getComponent<Mesh>()->load();
    };
  }
};

} // namespace Magma
