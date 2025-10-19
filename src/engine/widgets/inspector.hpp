#pragma once
#include "../gameobject.hpp"
#include "inspector_menu.hpp"
#include "scene_menu.hpp"
#include "widget.hpp"

namespace Magma {

class Device;

/**
 * Inspector widget for viewing and editing a selected GameObject.
 * @note Only one GameObject can be inspected at a time.
 * @note If no GameObject is selected, the inspector will be empty.
 */
class Inspector : public Widget {
public:
  Inspector(Device *device): device{device} {}
  const char *name() const override { return "Inspector"; }

  /**
   * Set the context GameObject to inspect
   * @note Passing nullptr clears the inspector
   */
  static void setContext(GameObject *obj) { contextTarget = obj; }

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
  InspectorMenu inspectorMenu = {};

  inline static GameObject *contextTarget = nullptr;
  Device *device = nullptr;
};

} // namespace Magma
