#pragma once
#include "components/camera.hpp"
#include "gameobject.hpp"
#include <functional>
#include <memory>
#include <vector>

namespace Magma {

class Renderer;

/**
 * Represents a scene containing multiple
 * GameObjects and manages their lifecycle and rendering.
 * @note Only one scene can be active at a time.
 */
class Scene {
public:
  Scene();
  ~Scene();

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  Scene(Scene &&) = default;
  Scene &operator=(Scene &&) = default;

  // --- Global access ---
  static Scene *current() { return activeScene; }
  void setActive() { activeScene = this; }

  static void setActiveCamera(Camera *camera) { activeCamera = camera; }
  static Camera *getActiveCamera() { return activeCamera; }

  std::vector<std::unique_ptr<GameObject>> &getGameObjects() {
    return gameObjects;
  }

  // Find GameObject by ID
  GameObject *findGameObjectById(GameObject::id_t id);

  // --- GameObject management ---
  GameObject &addGameObject(std::unique_ptr<GameObject> gameObject);
  void removeGameObject(GameObject *gameObject);

// --- Scene operations ---
/**
 * Draw the scene hierarchy in a tree structure
 * */
#if defined(MAGMA_WITH_EDITOR)
  static void drawTree();
#endif

  /**
   * Render GameObjects recursively
   * @param renderer Renderer to use for rendering
   */
  static void onRender(Renderer &renderer);

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
  void processDeferredActions();

private:
  inline static Scene *activeScene = nullptr;
  inline static Camera *activeCamera = nullptr;

  std::vector<std::unique_ptr<GameObject>> gameObjects;
  std::vector<std::function<void()>> deferredActions;
};

} // namespace Magma
