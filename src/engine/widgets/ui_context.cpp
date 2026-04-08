module;
#include "imgui.h"

export module widgets:ui_context;

namespace Magma {

export struct UIContext {
  inline static bool initialized = false;
  inline static ImGuiWindowClass AppDockClass{};        // Root/app dockspace
  inline static ImGuiWindowClass InspectorDockClass{};  // Inspector-only dockspace
  inline static ImGuiWindowClass GameViewDockClass{};   // Game view dockspace

  inline static ImGuiID TopBarDockId = 0;

  static void ensureInit() {
    if (initialized) return;
    // Assign stable class IDs to filter docking
    AppDockClass.ClassId = ImGui::GetID("Magma_AppDockClass");
    InspectorDockClass.ClassId = ImGui::GetID("Magma_InspectorDockClass");
    InspectorDockClass.ClassId = ImGui::GetID("Magma_GameViewDockClass");
    initialized = true;
  }
};

} // namespace Magma
