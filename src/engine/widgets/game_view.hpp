#pragma once
#include "engine/render/viewport.hpp"
#include "widget.hpp"

namespace Magma {

class SceneRenderer;
class GameObject;

class GameView : public Widget {
public:
  explicit GameView(Viewport viewport)
      : viewport{viewport} {}

  const char *name() const override { return "Game"; }

  // Perform resize decision before starting frame.
  void preFrame() override;
  void draw() override;

  // Docking prefrence (Center)
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Center, 0.f};
  }

private:
  Viewport viewport;
};

} // namespace Magma
