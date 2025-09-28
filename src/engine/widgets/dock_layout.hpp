#pragma once
#include "imgui.h"
#include "imgui_internal.h"

namespace Magma {

class DockLayout {
public:
  DockLayout(ImGuiID rootId, const ImVec2 &size,
             ImGuiDockNodeFlags rootFlags = ImGuiDockNodeFlags_DockSpace);

  // Splitting
  ImGuiID splitLeft(float ratio);
  ImGuiID splitRight(float ratio);
  ImGuiID splitUp(float ratio);
  ImGuiID splitDown(float ratio);
  ImGuiID centerNode();

  // Docking
  void makeCentral();
  void dockWindow(const char *name, ImGuiID nodeId);
  void finish();

private:
  ImGuiID rootId;
  ImGuiID mainId;

  // Width calculations
  float remainingWidthRatio = 1.f;
  float toRelativeWidth(float ratio) const;

  // Height calculations
  float remainingHeightRatio = 1.f;
  float toRelativeHeight(float ratio) const;
};

} // namespace Magma
