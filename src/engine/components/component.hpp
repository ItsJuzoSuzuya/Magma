#pragma once

namespace Magma {

class Renderer;

class Component {
public:
  Component() = default;
  virtual ~Component() = default;

  // Lifecycle
  virtual void onRender(Renderer &renderer) = 0;
  virtual void onAwake() = 0;
  virtual void onUpdate() = 0;
};

} // namespace Magma
