#pragma once

#include "widget.hpp"
namespace Magma {

class GameObject;
class Device;

/**
  * Menu context state
  */
struct MenuContext {
  GameObject *target = nullptr;
  Device *device = nullptr;
};

/**
 * Inspector Menu Widget
 */
class InspectorMenu : public Widget {
public:
  const char *name() const override { return "Inspector Menu"; }

  // Setters
  static void setContextTarget(GameObject *gameObject) {
    contextState.target = gameObject;
  }

  /**
   * Queue opening Menu 
   */
  static void queueContextMenuFor(MenuContext state) {
    contextState = state;
    openPopupRequested = true;
  }

  // Render
  void draw() override;

private:
  inline static MenuContext contextState = {};
  inline static bool openPopupRequested = false;

  void drawAddComponentMenu();
};
} // namespace Magma
