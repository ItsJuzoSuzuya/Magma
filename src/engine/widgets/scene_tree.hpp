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

  // Queue opening the context menu at window-root scope this frame
  static void queueContextMenuFor(GameObject *gameObject) {
    contextTarget = gameObject;
    openPopupRequested = true;
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
  inline static bool openPopupRequested = false;
};

} // namespace Magma
