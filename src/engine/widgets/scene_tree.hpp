#pragma once
#include "scene_menu.hpp"
#include "widget.hpp"

namespace Magma {

class SceneTree : public Widget {
public:
  // Name of the widget
  const char *name() const override { return "Scene Tree"; }

  // Lifecycle
  void preFrame() override;
  void draw() override;

  // Docking prefrence (Left 25%)
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Left, 0.25f};
  }

private:
  // Scene Menu
  SceneMenu sceneMenu = {};
};

} // namespace Magma
