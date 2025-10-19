#pragma once
#include "imgui.h"

namespace Magma {

struct UIContext {
  inline static bool initialized = false;
  inline static ImGuiWindowClass AppDockClass{};        // Root/app dockspace
  inline static ImGuiWindowClass InspectorDockClass{};  // Inspector-only dockspace

  static void ensureInit() {
    if (initialized) return;
    // Assign stable class IDs to filter docking
    AppDockClass.ClassId = ImGui::GetID("Magma_AppDockClass");
    InspectorDockClass.ClassId = ImGui::GetID("Magma_InspectorDockClass");
    initialized = true;
  }
};

} // namespace Magma
