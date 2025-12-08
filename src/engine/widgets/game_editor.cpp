#include "game_editor.hpp"
#include "../components/transform.hpp"
#include "../render/offscreen_renderer.hpp"
#include "../time.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "inspector.hpp"
#include "ui_context.hpp"
#include <algorithm>
#include <cmath>
#include <glm/fwd.hpp>
#include <print>

using namespace std;
namespace Magma {

static ImVec2 fit16x9(const ImVec2 &avail) {
  // Maintain 16:9 inside the available region
  // width/height = 16/9 => height = width * 9/16
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

  // Clamp to at least 1x1
  size.x = (float)ImMax(1, (int)size.x);
  size.y = (float)ImMax(1, (int)size.y);
  return size;
}

// Perform resize decision before starting frame.
void GameEditor::preFrame() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::GameViewDockClass);
  bool open = ImGui::Begin(name());
  if (open) {
    ImVec2 avail = ImGui::GetContentRegionAvail();
    ImVec2 desired = fit16x9(avail);

    ImVec2 current = offscreenRenderer.getSceneSize();
    bool needsResize = ((int)desired.x != (int)current.x) ||
                       ((int)desired.y != (int)current.y);

    if (needsResize) {
      VkExtent2D newExtent{(uint32_t)desired.x, (uint32_t)desired.y};
      offscreenRenderer.resize(newExtent);

      if (editorCamera) {
        float aspect = desired.x > 0.f ? desired.x / desired.y : 16.f / 9.f;
        editorCamera->setAspectRatio(aspect);
      }
    }
  }

  ImGui::End();
}

// Draw the widget
void GameEditor::draw() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::GameViewDockClass);
  bool open = ImGui::Begin("Editor");

  if (!open) {
    ImGui::End();
    return;
  }

  if (ImGui::IsWindowFocused(ImGuiFocusedFlags_RootAndChildWindows)) {
    const float cameraSpeed = 1.5f * Time::getDeltaTime();

    if (ImGui::IsKeyDown(ImGuiKey_D))
      editorCamera->moveRight(cameraSpeed);
    if (ImGui::IsKeyDown(ImGuiKey_A))
      editorCamera->moveRight(-cameraSpeed);
    if (ImGui::IsKeyDown(ImGuiKey_W))
      editorCamera->moveForward(cameraSpeed);
    if (ImGui::IsKeyDown(ImGuiKey_S))
      editorCamera->moveForward(-cameraSpeed);
    if (ImGui::IsKeyDown(ImGuiKey_Space))
      editorCamera->moveUp(cameraSpeed);
    if (ImGui::IsKeyDown(ImGuiKey_LeftShift))
      editorCamera->moveUp(-cameraSpeed);
  }

  ImVec2 avail = ImGui::GetContentRegionAvail();
  ImVec2 imgSize = fit16x9(avail);

  ImGui::Image(offscreenRenderer.getSceneTexture(), imgSize);

  ImVec2 imageMin = ImGui::GetItemRectMin();
  ImVec2 imageMax = ImGui::GetItemRectMax();

  if (ImGui::IsItemClicked()) {
    ImVec2 mousePos = ImGui::GetIO().MousePos;
    float localX = mousePos.x - imageMin.x;
    float localY = mousePos.y - imageMin.y;

    localX = clamp(localX, 0.0f, imgSize.x);
    localY = clamp(localY, 0.0f, imgSize.y);

    uint32_t pixelX = static_cast<uint32_t>(localX);
    uint32_t pixelY = static_cast<uint32_t>(localY);

    if (imgSize.x > 0 && imgSize.y > 0) {
      GameObject *picked = offscreenRenderer.pickAtPixel(pixelX, pixelY);

      if (picked) {
        beginDrag(picked, mousePos, imageMin, imgSize);
        Inspector::setContext(picked);
      }
    }
  }

  if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
    draggedObject = nullptr;
    dragPixelOffset = ImVec2{0, 0};
    dragStartMousePos = ImVec2{0, 0};
  }

  handleMouseDrag();

  ImGui::End();
}

// --- Private --- //

void GameEditor::beginDrag(GameObject *picked, const ImVec2 &mousePos,
                           const ImVec2 &imageMin, const ImVec2 &imageSize) {
  draggedObject = picked;
  dragStartMousePos = mousePos;
  dragStartImageMin = imageMin;
  dragStartImageSize = imageSize;

  auto t = picked->getComponent<Transform>();
  if (!t || !editorCamera) {
    dragStartWorldPos = glm::vec3(0.f);
    dragPixelOffset = ImVec2(0, 0);
    dragStartNDCDepth = 0.f;
    return;
  }

  dragStartWorldPos = t->position;

  glm::mat4 proj = editorCamera->getProjection();
  glm::mat4 view = editorCamera->getView();
  glm::mat4 projView = proj * view;

  // Project object to clip space
  glm::vec4 clip = projView * glm::vec4(dragStartWorldPos, 1.f);
  if (clip.w == 0.f) {
    dragPixelOffset = ImVec2(0, 0);
    dragStartNDCDepth = 0.f;
    return;
  }
  glm::vec3 ndc =
      glm::vec3(clip) /
      clip.w; // [-1,1] for x,y ; [0,1] or [-1,1] for z depending on projection

  dragStartNDCDepth = ndc.z;

  // Convert NDC to pixel (local inside image)
  // NDC x: -1 -> 0px, +1 -> width
  float objPixelX = (ndc.x * 0.5f + 0.5f) * imageSize.x;
  // NDC y: +1 (top) -> 0px, -1 (bottom) -> height (because of inverted
  // viewport)
  float objPixelY = (1.f - (ndc.y * 0.5f + 0.5f)) * imageSize.y;

  // Mouse local pixel
  float mouseLocalX = mousePos.x - imageMin.x;
  float mouseLocalY = mousePos.y - imageMin.y;

  dragPixelOffset = ImVec2(mouseLocalX - objPixelX, mouseLocalY - objPixelY);
}

void GameEditor::handleMouseDrag() {
  if (!draggedObject || !editorCamera)
    return;

  ImGuiIO &io = ImGui::GetIO();
  ImVec2 mousePos = io.MousePos;

  // Current local pixel inside image
  float localX = mousePos.x - dragStartImageMin.x;
  float localY = mousePos.y - dragStartImageMin.y;

  // Clamp to image bounds
  localX = std::clamp(localX, 0.f, dragStartImageSize.x);
  localY = std::clamp(localY, 0.f, dragStartImageSize.y);

  // Desired object pixel (account for initial offset)
  float desiredObjPixelX = localX - dragPixelOffset.x;
  float desiredObjPixelY = localY - dragPixelOffset.y;

  // Clamp desired object pixel (optional)
  desiredObjPixelX = std::clamp(desiredObjPixelX, 0.f, dragStartImageSize.x);
  desiredObjPixelY = std::clamp(desiredObjPixelY, 0.f, dragStartImageSize.y);

  // Convert desired pixel to NDC
  float ndcX = (desiredObjPixelX / dragStartImageSize.x) * 2.f - 1.f;
  float ndcY = 1.f - (desiredObjPixelY / dragStartImageSize.y) * 2.f;
  float ndcZ = dragStartNDCDepth;

  glm::mat4 proj = editorCamera->getProjection();
  glm::mat4 view = editorCamera->getView();
  glm::mat4 invProjView = glm::inverse(proj * view);

  glm::vec4 ndcPos(ndcX, ndcY, ndcZ, 1.f);
  glm::vec4 worldH = invProjView * ndcPos;

  if (worldH.w == 0.f)
    return;

  glm::vec3 worldPos = glm::vec3(worldH) / worldH.w;

  if (auto t = draggedObject->getComponent<Transform>())
    t->position = worldPos;
}

} // namespace Magma
