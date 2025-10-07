#pragma once

#include "widget.hpp"
namespace Magma {

class GameObject;

class SceneMenu : public Widget {
public:
  const char *name() const override { return "Scene Menu"; }

  // Getters
  static GameObject *getContextTarget() { return contextTarget; }

  // Setters
  static void setContextTarget(GameObject *gameObject) {
    contextTarget = gameObject;
  }

  // Queue opening the context menu at window-root scope this frame
  static void queueContextMenuFor(GameObject *gameObject) {
    contextTarget = gameObject;
    openPopupRequested = true;
  }

  // Render
  void draw() override;

private:
  inline static GameObject *contextTarget = nullptr;
  inline static bool openPopupRequested = false;
};
} // namespace Magma
