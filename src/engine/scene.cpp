module;

#include <algorithm>
#include <cassert>
#include <functional>
#include <memory>
#include <vector>

export module engine:scene;
import core;
import :gameobject;

namespace Magma {

export class Scene {
public:
  Scene() {
    if (activeScene == nullptr)
      setActive();
  }

  ~Scene(){
    Device::waitIdle();

    if (activeScene == this)
      activeScene = nullptr;
  }

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  Scene(Scene &&) = default;
  Scene &operator=(Scene &&) = default;

  static Scene *current() { return activeScene; }
  void setActive() { activeScene = this; }


  std::vector<std::unique_ptr<GameObject>> &getGameObjects(){
    return gameObjects;
  }

  GameObject &addGameObject(std::unique_ptr<GameObject> gameObject, bool hidden = false){
    assert(gameObject != nullptr &&
           "GameObject cannot be null when adding to scene");

    GameObject &ref = *gameObject;
    gameObjects.push_back(std::move(gameObject));
    return ref;
  }

  void removeGameObject(GameObject *gameObject){
    if (!gameObject) return;

    defer([this, gameObject]() {
      if (gameObject->parent) {
        gameObject->parent->removeChild(gameObject);
      } else {
        gameObjects.erase(
            std::remove_if(gameObjects.begin(), gameObjects.end(),
                           [&](const auto &go) { return go.get() == gameObject; }),
            gameObjects.end());
      }
    });
  }

  /**
   * Defers an action to be executed after the current frame
   */
  void defer(std::function<void()> func) { deferredActions.push_back(func); }

  /**
   * Process deferred actions queued during the frame
   */
  void processDeferredActions(){
    if (deferredActions.empty())
      return;

    Device::waitIdle();
    for (auto &action : deferredActions)
      action();

    deferredActions.clear();
  }

  inline static Scene *activeScene = nullptr;

private:
  std::vector<std::unique_ptr<GameObject>> gameObjects;
  std::vector<std::function<void()>> deferredActions;
};

}
