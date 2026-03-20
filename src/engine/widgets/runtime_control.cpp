module;
#include "imgui.h"

module wigets:runtime_control;
import std;

namespace Magma {

/**
 * @brief A runtime control widget for the ImGuiRenderer.
 * This widget provides controls for managing runtime aspects of the application,
 * such as starting, stopping, and pausing the simulation or game.
 */
export class RuntimeControl: public Widget {
public:
  enum RunState {
    Stopped,
    Running,
    Paused
  };

  // Name of the widget
  const char *name() const override { return "Runtime Control"; }

  // Draw the widget
  void draw() override {
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

  // Docking preference: place the runtime control on the top
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Up, 0.01f};
  }

  // Get the current runtime state
  static RunState getState() {
    return state;
  }
  
private:
  inline static RunState state = Stopped;
  void beginPlaySession();
  void endPlaySession();

  void drawButtons() {
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
};

}
