#pragma once
#include "widget.hpp"

namespace Magma {

class OffscreenRenderer;
class GameObject;

class GameView : public Widget {
public:
  explicit GameView(OffscreenRenderer &renderer)
      : offscreenRenderer{renderer} {}

  // Name of the widget
  const char *name() const override { return "Game"; }

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
};

} // namespace Magma
