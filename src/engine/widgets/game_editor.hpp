#pragma once
#include "imgui.h"
#include "../editor_camera.hpp"
#include "widget.hpp"

namespace Magma {

class OffscreenRenderer;
class GameObject;

class GameEditor : public Widget {
public:
  explicit GameEditor(OffscreenRenderer &renderer, EditorCamera *editorCamera = nullptr)
      : offscreenRenderer{renderer}, editorCamera{editorCamera} {}

  // Name of the widget
  const char *name() const override { return "Editor"; }

  // Perform resize decision before starting frame.
  void preFrame() override;

  // Draw the widget
  void draw() override;

  // Docking prefrence (Center)
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Center, 0.f};
  }

private:
  OffscreenRenderer &offscreenRenderer;
  EditorCamera *editorCamera = nullptr;

  // Drag state
  GameObject *draggedObject = nullptr;
  ImVec2 dragStartMousePos{0,0};
  ImVec2 dragStartImageMin{0,0};
  ImVec2 dragStartImageSize{0,0};

  // World position at start of drag
  glm::vec3 dragStartWorldPos{0.f,0.f,0.f};

  // Pixel offset between mouse and object projected pixe (mouse - objectPixel)
  ImVec2 dragPixelOffset{0,0};

  // Depth in NDC (clip.z / clip.w) at start of drag
  float dragStartNDCDepth = 0.f;

  void beginDrag(GameObject* object, const ImVec2& mousePos, const ImVec2& imageMin, const ImVec2& imageSize);
  void handleMouseDrag();
};

} // namespace Magma
