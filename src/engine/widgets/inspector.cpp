#include "inspector.hpp"
#include "../gameobject.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "inspector_menu.hpp"
#include "ui_context.hpp"
#include <cfloat>
#include <cstdint>
#include <print>

using namespace std;
namespace Magma {

// --- Public --- //
// --- Rendering & Drawing ---
bool Inspector::preFrame() {
  UIContext::ensureInit();
  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin(name());
  ImGui::End();
  return true;
}

void Inspector::draw() {
  UIContext::ensureInit();

  ImGuiID innerDockspaceId = ImGui::GetID("InspectorDockSpace");

  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin(name());

  const ImGuiHoveredFlags hoveredFlags =
      ImGuiHoveredFlags_ChildWindows |
      ImGuiHoveredFlags_DockHierarchy |
      ImGuiHoveredFlags_NoPopupHierarchy;

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(hoveredFlags)) {
    MenuContext context;
    context.target = contextTarget;
    context.device = device;
    InspectorMenu::queueContextMenuFor(context);
  }

  vector<Component *> components;
  if (contextTarget)
    components = contextTarget->getComponents();

  if (components.empty()) {
    ImGui::TextDisabled("No components to inspect.");
    ImGui::End();
    inspectorMenu.draw();
    return;
  }

  float totalHeight = 0.f;
  for (auto *component : components) 
    totalHeight += component->inspectorHeight();

  ImGui::BeginChild("##InspectorDockScroll", ImVec2(-FLT_MIN, totalHeight), false,
                    ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
  
  ImVec2 childAvail = ImGui::GetContentRegionAvail();
  float dockWidth = childAvail.x;

  ImGui::DockSpace(innerDockspaceId, ImVec2(0.f, 0.f),
                   ImGuiDockNodeFlags_None, &UIContext::InspectorDockClass);

  static GameObject* lastTarget = nullptr;
  static int lastCount = -1;
  static float lastTotalHeight = 0.f;
  vector<ImGuiID> lastDockedWindows = {static_cast<uint32_t>(components.size())};

  const bool targetChanged = (lastTarget != contextTarget);
  const bool countChanged = (static_cast<int>(components.size()) != lastCount);
  const bool heightChanged = std::fabs(totalHeight - lastTotalHeight) > 1.f;

  if (targetChanged || countChanged || heightChanged) {
    ImGui::DockBuilderRemoveNode(innerDockspaceId);
    ImGui::DockBuilderAddNode(innerDockspaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_AutoHideTabBar);

    ImVec2 nodeSize = ImVec2(dockWidth, totalHeight);
    if (nodeSize.x <= 0.0f || nodeSize.y <= 0.0f) 
      nodeSize = ImVec2(1.f, 1.f);
    ImGui::DockBuilderSetNodeSize(innerDockspaceId, nodeSize);

    lastDockedWindows.clear();

    if (!components.empty() && totalHeight > 0.0f) {
      ImGuiID remainingId = innerDockspaceId;
      float remainingPx = totalHeight;

      for (size_t i = 0; i < components.size(); ++i) {
        const char* winName = components[i]->inspectorName();
        const float compHeight = components[i]->inspectorHeight();

        if (remainingPx <= 1.0f) break;
        println("Remaining px: {}", remainingPx);
        println("  Docking component '{}' with height {}", winName, compHeight);

        // Extract a top slice with height ~= desiredPx from the remaining space
        float ratio = compHeight / remainingPx;
        if (ratio <= 0.0f) break;
        if (ratio >= 0.999f && i + 1 < components.size()) ratio = 0.999f; // keep space for next slices

        ImGuiID topSlice = 0;
        ImGui::DockBuilderSplitNode(remainingId , ImGuiDir_Up, ratio, &topSlice, &remainingId);
        ImGui::DockBuilderDockWindow(winName, topSlice);
        lastDockedWindows.push_back(topSlice);

        remainingPx -= compHeight;
      }
    }

    ImGui::DockBuilderFinish(innerDockspaceId);
    lastTarget = contextTarget;
    lastCount = static_cast<int>(components.size());
    lastTotalHeight = totalHeight;
  }
  ImGui::EndChild();
  ImGui::End();

  if (!components.empty()) {
    for (int i = 0; i < components.size(); ++i) {
      Component* component = components[i];
      ImGui::SetNextWindowClass(&UIContext::InspectorDockClass);

      // If we have a node id for this component, dock into that node specifically.
      if (i < lastDockedWindows.size()) 
        ImGui::SetNextWindowDockID(lastDockedWindows[i], ImGuiCond_Appearing);
      else 
        // Fallback: dock into the main dockspace (will group with others)
        ImGui::SetNextWindowDockID(innerDockspaceId, ImGuiCond_Appearing);

      const float h = component->inspectorHeight();
      ImGui::SetNextWindowSize(ImVec2(dockWidth, h), ImGuiCond_Appearing);
      ImGui::SetNextWindowSizeConstraints(ImVec2(200.f, h), ImVec2(FLT_MAX, FLT_MAX));

      const char* title = component->inspectorName();
      if (ImGui::Begin(title, nullptr, ImGuiWindowFlags_NoSavedSettings)) 
        component->onInspector();
      ImGui::End();
    }
  }

  inspectorMenu.draw();
}

} // namespace Magma
