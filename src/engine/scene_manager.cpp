module;
#include <cstdint>
#include <functional>
#include <typeinfo>
#include <vector>
#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
#endif

export module engine:scene_manager;
import :scene;
import :gameobject;
import components;

namespace Magma {

export class SceneManager {
public:
  inline static Scene *activeScene = nullptr;
  inline static GameObject *activeCamera = nullptr;

  template <typename T>
  static T *getComponentFromGameObject(uint64_t id) {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must be a Component");

    GameObject *obj = findGameObjectById(id);
    if (!obj)
      return nullptr;

    return obj->getComponent<T>();
  }

  static GameObject *findGameObjectById(uint64_t id){
    if (activeScene == nullptr)
      return nullptr;

    std::function<GameObject *(GameObject *)> findObjectInChildren =
        [&](GameObject *node) -> GameObject * {
      if (!node)
        return nullptr;

      if (node->id == id)
        return node;

      auto children = node->getChildren();
      for (auto *child : children) {
        if (auto obj = findObjectInChildren(child))
          return obj;
      }
      return nullptr;
    };

    for (const auto &go : activeScene->getGameObjects()) {
      if (!go)
        continue;

      if (go->id == id)
        return go.get();

      if (auto obj = findObjectInChildren(go.get()))
        return obj;
    }
    return nullptr;
  }
};

} // namespace Magma
