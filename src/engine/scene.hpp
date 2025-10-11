#pragma once
#include "gameobject.hpp"
#include <memory>
#include <vector>

namespace Magma {

class Renderer;

class Scene {
public:
  Scene();
  ~Scene();

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  Scene(Scene &&) = default;
  Scene &operator=(Scene &&) = default;

  // Getters
  static Scene *current() { return activeScene; }

  // Setters
  void setActive() { activeScene = this; }

  // GameObjects
  void addGameObject();
  void addGameObject(std::unique_ptr<GameObject> gameObject);

  // Draw scene tree
  static void drawTree();

  // Render GameObjects
  static void onRender(Renderer &renderer);
  static void draw(Renderer &renderer);

private:
  // Active scene for static access
  inline static Scene *activeScene = nullptr;

  // Root GameObjects
  std::vector<std::unique_ptr<GameObject>> gameObjects;
};

} // namespace Magma
