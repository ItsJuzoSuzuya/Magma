#pragma once
#include "engine/gameobject.hpp"
#include "engine/scene.hpp"
#include <cstdint>
#include <memory>
#include <vector>

namespace Magma {

class SceneManager {
public:
  inline static Scene *activeScene = nullptr;
  inline static std::vector<std::unique_ptr<Scene>> scenes = {};

  inline static Scene *createScene(std::string name = "Scene"){
    auto scene = std::make_unique<Scene>(name);
    auto scenePtr = scene.get();
    scenes.emplace_back(std::move(scene));
    return scenePtr;
  }

  template <typename T>
  static T *getComponentFromGameObject(uint64_t id) {
    static_assert(std::is_base_of<Component, T>::value, "T must be a Component");
    GameObject *obj = findGameObjectById(id);
    if (!obj) return nullptr;
    return obj->getComponent<T>();
  }

  static GameObject *findGameObjectById(uint64_t id);
};

} // namespace Magma
