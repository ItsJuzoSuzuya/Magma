#pragma once
#include "imgui.h"
#include "widget.hpp"

namespace Magma {

class OffscreenRenderer;
class GameObject;

class OffscreenView : public Widget {
public:
  explicit OffscreenView(OffscreenRenderer &renderer)
      : offscreenRenderer{renderer} {}

  // Name of the widget
  const char *name() const override { return "Offscreen View"; }

  // Perform resize decision before starting frame.
  void preFrame() override;

  // Draw the widget
  void draw() override;

  // Docking prefrence (Center)
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Center, 0.f};
  }

private:
  OffscreenRenderer &offscreenRenderer;

  GameObject *draggedObject = nullptr;
  ImVec2 dragStartMousePos = {0,0};
  void handleMouseDrag();
};

} // namespace Magma
