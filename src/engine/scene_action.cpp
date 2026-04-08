module;
#include <algorithm>
#include <functional>

export module engine:scene_action;
import :gameobject;
import :scene_manager;

namespace Magma {

export class SceneAction {
public:
  inline static std::function<void()> remove(GameObject *obj) {
    return [obj]() {
      if (!obj)
        return;

      if (auto parent = obj->parent) {
        parent->removeChild(obj);
      } else if (auto scene = SceneManager::activeScene) {
        auto &gameObjects = scene->getGameObjects();
        gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                           [&](const auto &go) { return go.get() == obj; }),
            gameObjects.end());
      }
    };
  }

  inline static std::function<void()> loadMesh(Mesh *mesh) {
    return [mesh]() {
      mesh->load();
    };
  }
};

} // namespace Magma
