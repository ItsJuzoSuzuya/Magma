#pragma once

#include "widget.hpp"
namespace Magma {

class GameObject;

/**
 * Inspector Menu Widget
 */
class InspectorMenu : public Widget {
public:
  const char *name() const override { return "Inspector Menu"; }

  /**
   * Queue opening Menu 
   */
  static void queueContextMenuFor(GameObject *target) {
    contextTarget = target;
    openPopupRequested = true;
  }

  // Render
  void draw() override;

private:
  inline static GameObject *contextTarget = nullptr;
  inline static bool openPopupRequested = false;

  void drawAddComponentMenu();
};
} // namespace Magma
