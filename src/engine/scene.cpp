module;

#include <functional>
#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
#endif

export module engine:scene;
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
    defer(SceneAction::remove(gameObject));
  }

  /**
   * Defers an action to be executed after the current frame
   * @param func The function to be executed
   * @note This is useful for actions that modify the scene
   */
  void defer(std::function<void()> func) { deferredActions.push_back(func); }

  /**
   * Process deferred actions queued during the frame
   * @note This should be called at the end of each frame
   */
  void processDeferredActions(){
    if (deferredActions.empty())
      return;

    Device::waitIdle();
    for (auto &action : deferredActions)
      action();

    deferredActions.clear();
  }


private:
  std::vector<std::unique_ptr<GameObject>> gameObjects;
  std::vector<std::function<void()>> deferredActions;
};

}
