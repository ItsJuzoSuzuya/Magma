#pragma once
#include "../gameobject.hpp"
#include "inspector_menu.hpp"
#include "scene_menu.hpp"
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
  static void setContext(GameObject *obj) { contextTarget = obj; }

  // --- Rendering & Drawing ---
  /**
   * Simple pre-frame draw for calculating offscreen view size
   */
  void preFrame() override;
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
  /** 
   * The inspector context menu 
   * @note Open via right-click in the inspector area
   * */
  InspectorMenu inspectorMenu = {};

  /** The current GameObject displayed */
  inline static GameObject *contextTarget = nullptr;

  // Inspector Layout State
  GameObject *lastTarget = nullptr;
  int lastCount = 0;
  float lastTotalHeight = 0.0f;
  float lastTotalWidth = 0.0f;
  void resetLayoutState() {
    lastTarget = nullptr;
    lastCount = 0;
    lastTotalHeight = 0.0f;
    lastTotalWidth = 0.0f;
  }

};

} // namespace Magma
