#pragma once
#include "../gameobject.hpp"
#include "widget.hpp"

namespace Magma {

class SceneTree : public Widget {
public:
  // Name of the widget
  const char *name() const override { return "Scene Tree"; }

  // Getters
  static GameObject *getContextTarget() { return contextTarget; }

  // Setters
  static void setContextTarget(GameObject *gameObject) {
    contextTarget = gameObject;
  }

  // Lifecycle
  bool preFrame() override;
  void draw() override;

  // Docking prefrence (Left 25%)
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Left, 0.25f};
  }

private:
  inline static GameObject *contextTarget = nullptr;
};

} // namespace Magma
