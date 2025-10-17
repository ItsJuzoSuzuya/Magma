#pragma once
#include "../gameobject.hpp"
#include "widget.hpp"

namespace Magma {

/**
 * Inspector widget for viewing and editing a selected GameObject.
 * @note Only one GameObject can be inspected at a time.
 * @note If no GameObject is selected, the inspector will be empty.
 */
class Inspector : public Widget {
public:
  const char *name() const override { return "Inspector"; }

  /**
   * Set the context GameObject to inspect
   * @note Passing nullptr clears the inspector
   */
  static void setContext(GameObject *obj) { context = obj; }

  // --- Rendering & Drawing ---
  /**
   * Simple pre-frame draw for calculating offscreen view size
   */
  bool preFrame() override;
  /**
   * Draw the inspector contents
   */
  void draw() override;

  // --- Docking ---
  /** Docking hint to place the inspector on the right side */
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Right, 0.25f};
  }

private:
  inline static GameObject *context;
};

} // namespace Magma
