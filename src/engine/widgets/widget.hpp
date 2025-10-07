#pragma once

#include <optional>
namespace Magma {

enum class DockSide { Left, Right, Up, Down, Center };

struct DockHint {
  DockSide side{DockSide::Center};
  float ratio{0.25f}; // used for splits (ignored for Center)
};

class Widget {
public:
  virtual ~Widget() = default;

  // Name of the widget
  virtual const char *name() const = 0;

  // Optional pre-frame step (e.g, resizing). Return false to skip frame.
  virtual bool preFrame() { return true; }

  // Draw the widget
  virtual void draw() = 0;

  // Docking prefrence
  virtual std::optional<DockHint> dockHint() const { return std::nullopt; }
};

} // namespace Magma
