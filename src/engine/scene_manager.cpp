#include "scene_manager.hpp"

namespace Magma {

GameObject *SceneManager::findGameObjectById(uint64_t id) {
  if (!activeScene) return nullptr;

  std::function<GameObject *(GameObject *)> findInChildren =
      [&](GameObject *node) -> GameObject * {
    if (!node) return nullptr;
    if (node->id == id) return node;
    for (auto *child : node->getChildren()) {
      if (auto *found = findInChildren(child))
        return found;
    }
    return nullptr;
  };

  for (const auto &go : activeScene->getGameObjects()) {
    if (!go) continue;
    if (go->id == id) return go.get();
    if (auto *found = findInChildren(go.get())) return found;
  }
  return nullptr;
}

}
