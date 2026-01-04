#include "game_view.hpp"
#include "engine/render/scene_renderer.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui_context.hpp"
#include <glm/fwd.hpp>

namespace Magma {

static ImVec2 fit16x9(const ImVec2 &avail) {
  float targetH = avail.x * 9.0f / 16.0f;
  float targetW = avail.y * 16.0f / 9.0f;

  ImVec2 size;
  if (targetH <= avail.y) {
    size.x = floorf(avail.x);
    size.y = floorf(targetH);
  } else {
    size.x = floorf(targetW);
    size.y = floorf(avail.y);
  }

  size.x = (float)ImMax(1, (int)size.x);
  size.y = (float)ImMax(1, (int)size.y);
  return size;
}

// Perform resize decision before starting frame.
void GameView::preFrame() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::GameViewDockClass);

  bool open = ImGui::Begin(name());
  if (open) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 desired = fit16x9(avail);

    ImVec2 current = renderer.getSceneSize();
    bool needsResize = ((int)desired.x != (int)current.x) ||
                       ((int)desired.y != (int)current.y);

    if (needsResize) {
      VkExtent2D newExtent{(uint32_t)desired.x, (uint32_t)desired.y};
      renderer.onResize(newExtent);
    }
  }
  ImGui::End();
}

// Draw the widget
void GameView::draw() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::GameViewDockClass);
  ImGui::Begin(name());

  ImVec2 imgSize = renderer.getSceneSize();
  ImGui::Image(renderer.getSceneTexture(), imgSize);

  ImVec2 imageMin = ImGui::GetItemRectMin();
  ImVec2 imageMax = ImGui::GetItemRectMax();

  ImGui::End();
}

} // namespace Magma
