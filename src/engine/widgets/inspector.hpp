#pragma once
#include "../gameobject.hpp"
#include "widget.hpp"

namespace Magma {

class Inspector : public Widget {
public:
  const char *name() const override { return "Inspector"; }

  // Context
  static void setContext(GameObject *obj) { context = obj; }

  // Lifecycle
  bool preFrame() override;
  void draw() override;

  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Right, 0.25f};
  }

private:
  inline static GameObject *context;
};

} // namespace Magma
