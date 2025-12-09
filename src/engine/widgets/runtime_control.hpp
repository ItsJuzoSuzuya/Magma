#pragma once
#include "widget.hpp"

namespace Magma {

/**
 * @brief A runtime control widget for the ImGuiRenderer.
 * This widget provides controls for managing runtime aspects of the application,
 * such as starting, stopping, and pausing the simulation or game.
 */
class RuntimeControl: public Widget {
public:
  enum RunState {
    Stopped,
    Running,
    Paused
  };

  // Name of the widget
  const char *name() const override { return "Runtime Control"; }

  // Draw the widget
  void draw() override;

  // Docking preference: place the runtime control on the top
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Up, 0.01f};
  }

  // Get the current runtime state
  static RunState getState() {
    return state;
  }
  
private:
  inline static RunState state = Stopped;
  void beginPlaySession();
  void endPlaySession();

  void drawButtons();
};

}
