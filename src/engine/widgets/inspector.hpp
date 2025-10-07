#pragma once
#include "widget.hpp"

namespace Magma {

class Inspector : public Widget {
public:
  const char *name() const override { return "Inspector"; }

  // Lifecycle
  bool preFrame() override;
  void draw() override;

  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Right, 0.25f};
  }
};

} // namespace Magma
