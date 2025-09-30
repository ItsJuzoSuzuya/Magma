#pragma once
#include "gameobject.hpp"
#include <memory>
#include <vector>
namespace Magma {
class Scene {
public:
  Scene();
  ~Scene();

  // Getters
  static Scene *current() { return activeScene; }

  // GameObjects
  void addGameObject(std::unique_ptr<GameObject> gameObject);

private:
  static Scene *activeScene;
  std::vector<std::unique_ptr<GameObject>> gameObjects;
};

} // namespace Magma
