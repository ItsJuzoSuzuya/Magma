#pragma once
#include "gameobject.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace Magma {

class Scene {
public:
  Scene(std::string name): name{name}{}
  ~Scene() {}

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  Scene(Scene &&) = default;
  Scene &operator=(Scene &&) = default;

  std::vector<std::unique_ptr<GameObject>> &getGameObjects() { return gameObjects; }
  std::string getName(){
    return name;
  }

  GameObject *createGameObject();
  GameObject *createGameObject(std::string name);
  GameObject *createGameObject(GameObject* parent);
  GameObject *createGameObject(GameObject* parent, std::string name);
  GameObject *createPointLight(GameObject* parent, std::string name);

  GameObject *addGameObject(std::unique_ptr<GameObject> gameObject);
  void removeGameObject(GameObject *gameObject);

  void defer(std::function<void()> func) { deferredActions.push_back(func); }
  void processDeferredActions();

  inline static GameObject *activeCamera = nullptr;

private:
  std::string name;
  std::vector<std::unique_ptr<GameObject>> gameObjects;
  std::vector<std::function<void()>> deferredActions;
};

} // namespace Magma
