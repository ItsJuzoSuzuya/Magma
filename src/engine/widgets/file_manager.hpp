#pragma once
#include "widget.hpp"

namespace Magma {
class FileManager: public Widget {
public:
  const char *name() const override { return "File Manager"; }

  void draw() override;

  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Down, 0.3f};
  }
};

} // namespace Magma
