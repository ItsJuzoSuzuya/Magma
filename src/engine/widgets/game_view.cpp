#include "game_view.hpp"
#include "../render/offscreen_renderer.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui_context.hpp"
#include <glm/fwd.hpp>

using namespace std;
namespace Magma {

// Perform resize decision before starting frame.
void GameView::preFrame() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::GameViewDockClass);
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
void GameView::draw() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::GameViewDockClass);
  ImGui::Begin(name());

  ImVec2 imgSize = offscreenRenderer.getSceneSize();
  ImGui::Image(offscreenRenderer.getSceneTexture(),
               imgSize);

  ImVec2 imageMin = ImGui::GetItemRectMin();
  ImVec2 imageMax = ImGui::GetItemRectMax();

  ImGui::End();
}


} // namespace Magma
