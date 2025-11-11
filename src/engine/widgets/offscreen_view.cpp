#include "offscreen_view.hpp"
#include "../render/offscreen_renderer.hpp"
#include "../scene.hpp"
#include "../components/transform.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "inspector.hpp"
#include "ui_context.hpp"
#include <glm/fwd.hpp>

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

  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
    ImGuiIO& io = ImGui::GetIO();
    io.WantCaptureKeyboard = true;

    if (ImGui::IsKeyPressed(ImGuiKey_A)) 
      Scene::current()->moveCameraAlongRight(-0.1f);
    if (ImGui::IsKeyPressed(ImGuiKey_D)) 
      Scene::current()->moveCameraAlongRight(0.1f);
    if (ImGui::IsKeyPressed(ImGuiKey_W)) 
      Scene::current()->moveCameraAlongForward(0.1f);
    if (ImGui::IsKeyPressed(ImGuiKey_S))
      Scene::current()->moveCameraAlongForward(-0.1f);
    if (ImGui::IsKeyPressed(ImGuiKey_Space)) 
      Scene::current()->moveCameraAlongUp(0.1f);
    if (ImGui::IsKeyPressed(ImGuiKey_LeftShift))
      Scene::current()->moveCameraAlongUp(-0.1f);
  }

  ImVec2 imgSize = offscreenRenderer.getSceneSize();
  ImGui::Image(offscreenRenderer.getSceneTexture(),
               imgSize);

  if(ImGui::IsItemClicked()){
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

    ImVec2 sceneSize = offscreenRenderer.getSceneSize();
    if (sceneSize.x > 0 && sceneSize.y > 0){
      GameObject* picked = offscreenRenderer.pickAtPixel(pixelX, pixelY);

      if (picked) {
        draggedObject = picked;
        dragStartMousePos = mousePos;
        Inspector::setContext(picked);
      }
    }
  }

  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    draggedObject = nullptr;
    dragStartMousePos = ImVec2{0,0};
  }

  handleMouseDrag();

  ImGui::End();
}

// --- Private --- //

void OffscreenView::handleMouseDrag(){
  if (draggedObject) {
    ImVec2 currentMousePos = ImGui::GetIO().MousePos;
    ImVec2 delta{
        currentMousePos.x - dragStartMousePos.x,
        currentMousePos.y - dragStartMousePos.y
    };

    // Apply some sensitivity factor
    float sensitivity = 0.01f;
    glm::vec3& pos = draggedObject->getComponent<Transform>()->position;
    pos.x += delta.x * sensitivity;
    pos.y -= delta.y * sensitivity;

    // Update drag start position for next frame
    dragStartMousePos = currentMousePos;
  }
}

} // namespace Magma
