#include "offscreen_view.hpp"
#include "../render/offscreen_renderer.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "inspector.hpp"
#include "ui_context.hpp"
#include <print>

using namespace std;
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

  ImVec2 imgSize = offscreenRenderer.getSceneSize();
  ImGui::Image(offscreenRenderer.getSceneTexture(),
               imgSize);

  if(ImGui::IsItemClicked()){
    println("OffscreenView: Clicked at image");
    ImVec2 itemMin = ImGui::GetItemRectMin();
    ImVec2 itemMax = ImGui::GetItemRectMax();
    ImVec2 mousePos = ImGui::GetIO().MousePos;

    float localX = mousePos.x - itemMin.x;
    float localY = mousePos.y - itemMin.y;

    // Clamp
    if (localX < 0) localX = 0;
    if (localY < 0) localY = 0;
    if (localX > imgSize.x) localX = imgSize.x;
    if (localY > imgSize.y) localY = imgSize.y;

    uint32_t pixelX = static_cast<uint32_t>(localX);
    uint32_t pixelY = static_cast<uint32_t>(localY);

    println("OffscreenView: Local click at pixel ({}, {})", pixelX, pixelY);

    ImVec2 sceneSize = offscreenRenderer.getSceneSize();
    if (sceneSize.x > 0 && sceneSize.y > 0){
      GameObject* picked = offscreenRenderer.pickAtPixel(pixelX, pixelY);
      println("OffscreenView: Picked object: {}", picked ? picked->name : "None");

      if (picked) 
        Inspector::setContext(picked);
    }
  }

  ImGui::End();
}

} // namespace Magma
