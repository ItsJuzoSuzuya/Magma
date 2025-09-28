
#include "dock_layout.hpp"

namespace Magma {
DockLayout::DockLayout(ImGuiID rootId, const ImVec2 &size,
                       ImGuiDockNodeFlags rootFlags)
    : rootId(rootId), mainId(rootId) {
  ImGui::DockBuilderRemoveNode(rootId);
  ImGui::DockBuilderAddNode(rootId, rootFlags);
  ImGui::DockBuilderSetNodeSize(rootId, size);
}

// Splitting
ImGuiID DockLayout::splitLeft(float ratio) {
  float relativeRatio = toRelativeWidth(ratio);
  ImGuiID out_left;
  ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Left, relativeRatio, &out_left,
                              &mainId);
  remainingWidthRatio -= ratio;
  return out_left;
}

ImGuiID DockLayout::splitRight(float ratio) {
  float relativeRatio = toRelativeWidth(ratio);
  ImGuiID out_right;
  ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Right, relativeRatio, &out_right,
                              &mainId);
  remainingWidthRatio -= ratio;
  return out_right;
}

ImGuiID DockLayout::splitUp(float ratio) {
  float relativeRatio = toRelativeHeight(ratio);
  ImGuiID out_up;
  ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Up, relativeRatio, &out_up,
                              &mainId);
  remainingHeightRatio -= ratio;
  return out_up;
}

ImGuiID DockLayout::splitDown(float ratio) {
  float relativeRatio = toRelativeHeight(ratio);
  ImGuiID out_down;
  ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Down, relativeRatio, &out_down,
                              &mainId);
  remainingHeightRatio -= ratio;
  return out_down;
}

ImGuiID DockLayout::centerNode() { return mainId; }

// Docking

void DockLayout::makeCentral() {
  if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(mainId))
    node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
}

void DockLayout::dockWindow(const char *name, ImGuiID nodeId) {
  ImGui::DockBuilderDockWindow(name, nodeId);
}

void DockLayout::finish() { ImGui::DockBuilderFinish(rootId); }

// --- Private ---
// Relative conversion
float DockLayout::toRelativeWidth(float ratio) const {
  if (remainingWidthRatio <= 0.f)
    return 0.f;

  float rel = ratio / remainingWidthRatio;
  if (rel < 0.f)
    return 0.f;
  if (rel > 0.999f)
    return 0.999f;
  return rel;
}
float DockLayout::toRelativeHeight(float ratio) const {
  if (remainingHeightRatio <= 0.f)
    return 0.f;

  float rel = ratio / remainingHeightRatio;
  if (rel < 0.f)
    return 0.f;
  if (rel > 0.999f)
    return 0.999f;
  return rel;
};

} // namespace Magma
