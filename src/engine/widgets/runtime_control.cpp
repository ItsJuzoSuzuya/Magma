#include "runtime_control.hpp"
#include "../core/icons.hpp"
#include "imgui.h"
#include "ui_context.hpp"

using namespace std;
namespace Magma {

// Draw the widget
void RuntimeControl::draw() {
  UIContext::ensureInit();

  if (UIContext::TopBarDockId != 0)
      ImGui::SetNextWindowDockID(UIContext::TopBarDockId, ImGuiCond_Always);
  ImGui::SetNextWindowClass(&UIContext::AppDockClass);

  ImGuiWindowFlags flags =
      ImGuiWindowFlags_NoTitleBar |
      ImGuiWindowFlags_NoResize |
      ImGuiWindowFlags_NoMove |
      ImGuiWindowFlags_NoCollapse |
      ImGuiWindowFlags_NoSavedSettings |
      ImGuiWindowFlags_NoScrollbar |
      ImGuiWindowFlags_NoScrollWithMouse;

  ImGui::Begin(name(), nullptr, flags);
  drawButtons();
  ImGui::End();
}

// --- Private methods ---

void RuntimeControl::drawButtons() {
  ImGuiStyle& style = ImGui::GetStyle();

  // Decide which buttons to show based on state
  bool showPlay      = (state == RunState::Stopped || state == RunState::Paused);
  bool showPause     = (state == RunState::Running);
  bool showStop      = (state != RunState::Stopped);

  // Compute total width for centering
  int buttonCount =
      (showPlay ? 1 : 0) +
      (showPause ? 1 : 0) +
      (showStop ? 1 : 0);

  if (buttonCount == 0) return;

  const float size = 20.0f; 
  const float spacing = style.ItemSpacing.x;
  const float totalWidth = buttonCount * size + (buttonCount - 1) * spacing;
  const float regionWidth = ImGui::GetContentRegionAvail().x;
  float startX = (regionWidth - totalWidth) * 0.5f;
  if (startX < 0.f) startX = 0.f;

  ImGui::SetCursorPosX(ImGui::GetCursorPosX() + startX);

  if (showPlay) {
    if (ImGui::Button(ICON_PLAY, ImVec2(size, size))) 
      state = RunState::Running;
    ImGui::SameLine(0, spacing);
  }

  if (showPause) {
    if (ImGui::Button(ICON_PAUSE, ImVec2(size, size))) 
      state = RunState::Paused;
    if (showStop)
      ImGui::SameLine(0, spacing);
  }

  if (showStop) {
    if (ImGui::Button(ICON_STOP, ImVec2(size, size))) 
      state = RunState::Stopped;
  }
}

}
