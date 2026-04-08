module;
#include "imgui.h"
#include "imgui_internal.h"

export module widgets:inspector;
import :widget;
import engine:components:component;

namespace Magma {

/**
 * Inspector widget for viewing and editing a selected GameObject.
 * @note Only one GameObject can be inspected at a time.
 * @note If no GameObject is selected, the inspector will be empty.
 */
export class Inspector : public Widget {
public:
  const char *name() const override { return "Inspector"; }

  /**
   * Set the context GameObject to inspect
   * @note Passing nullptr clears the inspector
   */
  static void setContext(GameObject *obj) { contextTarget = obj; }

  // --- Rendering & Drawing ---
  /**
   * Simple pre-frame draw for calculating offscreen view size
   */
  void Inspector::preFrame() override {
    UIContext::ensureInit();
    ImGui::SetNextWindowClass(&UIContext::AppDockClass);
    ImGui::Begin(name());
    ImGui::End();
  }

  void Inspector::draw() override {
    UIContext::ensureInit();

    ImGuiID innerDockspaceId = ImGui::GetID("InspectorDockSpace");

    ImGui::SetNextWindowClass(&UIContext::AppDockClass);
    ImGui::Begin(name());

    // Early out if no target
    if (!contextTarget) {
      resetLayoutState();
      ImGui::TextDisabled("No target selected");
      ImGui::End();
      return;
    }

    // Right-click context menu
    const ImGuiHoveredFlags hoveredFlags =
        ImGuiHoveredFlags_ChildWindows |
        ImGuiHoveredFlags_DockHierarchy |
        ImGuiHoveredFlags_NoPopupHierarchy;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered(hoveredFlags)) 
      InspectorMenu::queueContextMenuFor(contextTarget);

    // Header
    ImGui::TextDisabled("Object: %s", contextTarget->name.c_str());

    // Gather components 
    vector<Component *> components;
    if (contextTarget)
      components = contextTarget->getComponents();

    // Early out if no components
    if (components.empty()) {
      lastTarget = contextTarget; 
      lastCount = 0;
      lastTotalHeight = 0.f;
      lastTotalWidth = 0.f;

      ImGui::End();
      inspectorMenu.draw();
      return;
    }

    // Calculate total height of all components
    float totalHeight = 0.f;
    for (auto *component : components) 
      totalHeight += component->inspectorHeight();

    // Scrolling child for dockspace
    ImGui::BeginChild("##InspectorDockScroll", ImVec2(0.f, totalHeight), false,
                      ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    ImVec2 childAvail = ImGui::GetContentRegionAvail();
    float dockWidth = childAvail.x;

    // Main dockspace
    ImGui::DockSpace(innerDockspaceId, ImVec2(0.f, 0.f),
                       ImGuiDockNodeFlags_None, &UIContext::InspectorDockClass);

    // ImGuiIDs
    vector<ImGuiID> lastDockedWindows;

    const bool targetChanged = (lastTarget != contextTarget);
    const bool countChanged = (static_cast<int>(components.size()) != lastCount);
    const bool heightChanged = (totalHeight != lastTotalHeight);
    const bool widthChanged = (dockWidth != lastTotalWidth);

    if (targetChanged || countChanged || heightChanged || widthChanged) {
      if (ImGui::DockBuilderGetNode(innerDockspaceId))
        ImGui::DockBuilderRemoveNode(innerDockspaceId);

      ImGui::DockBuilderAddNode(innerDockspaceId, ImGuiDockNodeFlags_DockSpace | ImGuiDockNodeFlags_AutoHideTabBar);

      ImVec2 nodeSize = ImVec2(dockWidth, totalHeight);
      if (nodeSize.x <= 0.0f || nodeSize.y <= 0.0f) 
        nodeSize = ImVec2(1.f, 1.f);
      ImGui::DockBuilderSetNodeSize(innerDockspaceId, nodeSize);

      lastDockedWindows.clear();

      if (!components.empty()) {
        ImGuiID remainingId = innerDockspaceId;
        float remainingPx = totalHeight;

        for (size_t i = 0; i < components.size(); ++i) {
          const char* winName = components[i]->inspectorName();
          const float compHeight = components[i]->inspectorHeight();

          if (remainingPx <= 1.0f) break;

          // Extract a top slice with height ~= desiredPx from the remaining space
          float ratio = compHeight / remainingPx;
          if (ratio <= 0.0f) break;
          if (ratio >= 0.999f && i + 1 < components.size()) ratio = 0.999f; // keep space for next slices

          ImGuiID topSlice = 0;
          if(i == components.size() - 1)
            topSlice = remainingId; // last one takes all remaining space
          else
            ImGui::DockBuilderSplitNode(remainingId , ImGuiDir_Up, ratio, &topSlice, &remainingId);

          ImGui::DockBuilderDockWindow(winName, topSlice);
          lastDockedWindows.push_back(topSlice);

          remainingPx -= compHeight;
        }

        ImGui::DockBuilderFinish(innerDockspaceId);
        lastTarget = contextTarget;
        lastCount = static_cast<int>(components.size());
        lastTotalHeight = totalHeight;
        lastTotalWidth = dockWidth;
      }
    }

    ImGui::EndChild();
    ImGui::End();

    if (!components.empty()) {
      for (int i = 0; i < components.size(); ++i) {
        Component* component = components[i];
        ImGui::SetNextWindowClass(&UIContext::InspectorDockClass);

        // If we have a node id for this component, dock into that node specifically.
        if (i  < (int)lastDockedWindows.size() - 1) 
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

  // --- Docking ---
  /** Docking hint to place the inspector on the right side */
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Right, 0.25f};
  }

private:
  /** 
   * The inspector context menu 
   * @note Open via right-click in the inspector area
   * */
  InspectorMenu inspectorMenu = {};

  /** The current GameObject displayed */
  inline static GameObject *contextTarget = nullptr;

  // Inspector Layout State
  GameObject *lastTarget = nullptr;
  int lastCount = 0;
  float lastTotalHeight = 0.0f;
  float lastTotalWidth = 0.0f;
  void resetLayoutState() {
    lastTarget = nullptr;
    lastCount = 0;
    lastTotalHeight = 0.0f;
    lastTotalWidth = 0.0f;
  }

};

} // namespace Magma
