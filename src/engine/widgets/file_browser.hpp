#pragma once
#include "widget.hpp"
#include <filesystem>
#include <string>

namespace Magma {

class FileBrowser: public Widget {
public:
  const char *name() const override { return "File Manager"; }

  void draw() override;

  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Down, 0.3f};
  }

private:
  std::string currentPath = std::filesystem::current_path();
};

} // namespace Magma
