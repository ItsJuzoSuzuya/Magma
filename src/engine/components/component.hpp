#pragma once

namespace Magma {

class GameObject;
class Renderer;

/**
 * Interface for all components.
 * Components are used to add functionality to entities.
 * @note All components must implement the Component interface.
 * @note Scripts are also considered components.
 */
class Component {
public:
  Component(GameObject *owner) : owner{owner} {}
  virtual ~Component() = default;

  // --- Lifecycle ---
  virtual void onAwake() = 0;
  virtual void onUpdate() = 0;
  virtual void onRender(Renderer &renderer) = 0;

  // Inspector
  virtual void onInspector() = 0;

protected:
  GameObject *owner = nullptr;
};

} // namespace Magma
