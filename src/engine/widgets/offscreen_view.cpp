#include "offscreen_view.hpp"
#include "../render/offscreen_renderer.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui_context.hpp"

namespace Magma {

// Perform resize decision before starting frame.
void OffscreenView::preFrame() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  bool open = ImGui::Begin(name());
  if (open) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 desired{
        (float)ImMax(1, (int)avail.x),
        (float)ImMax(1, (int)avail.y),
    };

    ImVec2 current = offscreenRenderer.getSceneSize();
    bool needsResize = ((int)desired.x != (int)current.x) ||
                       ((int)desired.y != (int)current.y);

    if (needsResize) {
      VkExtent2D newExtent{(uint32_t)desired.x, (uint32_t)desired.y};
      offscreenRenderer.resize(newExtent);
    }
  }

  ImGui::End();
}

// Draw the widget
void OffscreenView::draw() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin("Offscreen View");
  ImGui::Image(offscreenRenderer.getSceneTexture(),
               offscreenRenderer.getSceneSize());
  ImGui::End();
}

} // namespace Magma
