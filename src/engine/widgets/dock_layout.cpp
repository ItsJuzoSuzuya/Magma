module;
#include "imgui.h"
#include "imgui_internal.h"

export module widgets:dock_layout;

namespace Magma {

export class DockLayout {
public:
  DockLayout(ImGuiID rootId, const ImVec2 &size,
             ImGuiDockNodeFlags rootFlags = ImGuiDockNodeFlags_DockSpace): rootId(rootId), mainId(rootId) {
    if (ImGui::DockBuilderGetNode(rootId))
      ImGui::DockBuilderRemoveNode(rootId);

    ImGui::DockBuilderAddNode(rootId, rootFlags);
    ImGui::DockBuilderSetNodeSize(rootId, size);
  }

  // Splitting
  ImGuiID splitLeft(float ratio) {
    float relativeRatio = toRelativeWidth(ratio);
    ImGuiID out_left;
    ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Left, relativeRatio, &out_left,
                                &mainId);
    remainingWidthRatio -= ratio;
    return out_left;
  }

  ImGuiID splitRight(float ratio) {
    float relativeRatio = toRelativeWidth(ratio);
    ImGuiID out_right;
    ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Right, relativeRatio, &out_right,
                                &mainId);
    remainingWidthRatio -= ratio;
    return out_right;
  }

  ImGuiID splitUp(float ratio) {
    float relativeRatio = toRelativeHeight(ratio);
    ImGuiID out_up;
    ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Up, relativeRatio, &out_up,
                                &mainId);
    remainingHeightRatio -= ratio;
    return out_up;
  }

  ImGuiID splitDown(float ratio) {
    float relativeRatio = toRelativeHeight(ratio);
    ImGuiID out_down;
    ImGui::DockBuilderSplitNode(mainId, ImGuiDir_Down, relativeRatio, &out_down,
                                &mainId);
    remainingHeightRatio -= ratio;
    return out_down;
  }

  ImGuiID centerNode() { return mainId; }

  // Docking
  void makeCentral() {
    if (ImGuiDockNode *node = ImGui::DockBuilderGetNode(mainId))
      node->LocalFlags |= ImGuiDockNodeFlags_CentralNode;
  }

  void dockWindow(const char *name, ImGuiID nodeId) {
    ImGui::DockBuilderDockWindow(name, nodeId);
  }

  void finish() { ImGui::DockBuilderFinish(rootId); }


private:
  ImGuiID rootId;
  ImGuiID mainId;

  // Width calculations
  float remainingWidthRatio = 1.f;
  float toRelativeWidth(float ratio) const {
    if (remainingWidthRatio <= 0.f)
      return 0.f;

    float rel = ratio / remainingWidthRatio;
    if (rel < 0.f)
      return 0.f;
    if (rel > 0.999f)
      return 0.999f;
    return rel;
  }

  // Height calculations
  float remainingHeightRatio = 1.f;
  float toRelativeHeight(float ratio) const {
    if (remainingHeightRatio <= 0.f)
      return 0.f;

    float rel = ratio / remainingHeightRatio;
    if (rel < 0.f)
      return 0.f;
    if (rel > 0.999f)
      return 0.999f;
    return rel;
  }
};

} // namespace Magma
