#pragma once
#include "widget.hpp"

namespace Magma {

class SceneRenderer;
class GameObject;

class GameView : public Widget {
public:
  explicit GameView(SceneRenderer &renderer)
      : renderer{renderer} {}

  const char *name() const override { return "Game"; }

  // Perform resize decision before starting frame.
  void preFrame() override;
  void draw() override;

  // Docking prefrence (Center)
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Center, 0.f};
  }

private:
  SceneRenderer &renderer;
};

} // namespace Magma
