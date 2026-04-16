#pragma once
#include "engine/components/mesh.hpp"
#include "engine/gameobject.hpp"
#include "engine/scene_manager.hpp"
#include <algorithm>
#include <functional>

namespace Magma {

class SceneAction {
public:
  inline static std::function<void()> remove(GameObject *obj) {
    return [obj]() {
      if (!obj) return;

      if (obj->parent) {
        obj->parent->removeChild(obj);
      } else if (SceneManager::activeScene) {
        auto &gameObjects = SceneManager::activeScene->getGameObjects();
        gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                           [&](const auto &go) { return go.get() == obj; }),
            gameObjects.end());
      }
    };
  }

  inline static std::function<void()> loadMesh(Mesh *mesh) {
    return [mesh]() {
      if (mesh) mesh->load();
    };
  }
};

} // namespace Magma
