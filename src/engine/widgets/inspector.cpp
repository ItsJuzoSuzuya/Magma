#include "inspector.hpp"
#include "../gameobject.hpp"
#include "imgui.h"
#include "imgui_internal.h"
#include "inspector_menu.hpp"
#include "ui_context.hpp"

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

  ImGui::SetNextWindowClass(&UIContext::AppDockClass);
  ImGui::Begin(name());

  const ImGuiHoveredFlags hoveredFlags = 
      ImGuiHoveredFlags_AllowWhenOverlapped;

  if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(hoveredFlags)) {
    MenuContext context;
    context.target = contextTarget;
    context.device = device;
    InspectorMenu::queueContextMenuFor(context);
  }

  ImGuiID innerDockspaceId = ImGui::GetID("InspectorDockSpace");
  ImGui::DockSpace(innerDockspaceId, ImVec2(0.0f, 0.0f),
                   0, &UIContext::InspectorDockClass);

  static GameObject* lastTarget = nullptr;
  if (lastTarget != contextTarget) {
    // Rebuild docking for new selection: dock all component windows as a tab-stack
    ImGui::DockBuilderRemoveNode(innerDockspaceId);
    ImGui::DockBuilderAddNode(innerDockspaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_AutoHideTabBar);
    if (contextTarget) {
      for (auto *component : contextTarget->getComponents()) {
        // Window name stable across frames
        std::string winName = std::string(component->inspectorName());
        ImGui::DockBuilderDockWindow(winName.c_str(), innerDockspaceId);
      }
    }
    ImGui::DockBuilderFinish(innerDockspaceId);
    lastTarget = contextTarget;
  }

 if (contextTarget) {
    for (auto *component : contextTarget->getComponents()) {
      ImGui::SetNextWindowClass(&UIContext::InspectorDockClass);
      ImGui::SetNextWindowDockID(innerDockspaceId, ImGuiCond_Appearing);
      const char* title = component->inspectorName();
      if (ImGui::Begin(title)) {
        component->onInspector();
      }
      ImGui::End();
    }
  }

  inspectorMenu.draw();

  ImGui::End();
}

} // namespace Magma
