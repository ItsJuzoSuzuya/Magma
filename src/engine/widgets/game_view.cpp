#include "game_view.hpp"
#include "../render/offscreen_renderer.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "ui_context.hpp"
#include <glm/fwd.hpp>

using namespace std;
namespace Magma {

static ImVec2 fit16x9(const ImVec2 &avail) {
  float targetH = avail.x * 9.0f / 16.0f;
  float targetW = avail.y * 16.0f / 9.0f;

  ImVec2 size = avail;
  if (targetH <= avail.y) {
    size = ImVec2(avail.x, targetH);
  } else {
    size = ImVec2(targetW, avail.y);
  }

  size.x = (float)ImMax(1, (int)size.x);
  size.y = (float)ImMax(1, (int)size.y);
  return size;
}

// Perform resize decision before starting frame.
void GameView::preFrame() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::GameViewDockClass);
  ImGui::Begin(name());
  ImGui::End();
}

// Draw the widget
void GameView::draw() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::GameViewDockClass);
  ImGui::Begin(name());

  ImVec2 imgSize = offscreenRenderer.getSceneSize();
  ImGui::Image(offscreenRenderer.getSceneTexture(), imgSize);

  ImVec2 imageMin = ImGui::GetItemRectMin();
  ImVec2 imageMax = ImGui::GetItemRectMax();

  ImGui::End();
}

} // namespace Magma
