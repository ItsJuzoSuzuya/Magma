module;
#include "imgui.h"
#include "imgui_internal.h"
#include <memory>
#include <vector>

export module widgets:widget_manager;
import :dock_layout;
import :ui_context;
import :widget;

namespace Magma {

export class WidgetManager {
public:
  WidgetManager() = default;
  ~WidgetManager() = default;

  void addWidget(std::unique_ptr<Widget> widget) {
    widgets.push_back(std::move(widget));
  }

  void dock(){
    UIContext::ensureInit();

    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGuiID dockspace_id = ImGui::GetID("MainDockSpace");
    ImGui::DockSpaceOverViewport(dockspace_id, viewport);

    // Build once on first frame
    if (!dockBuilt) {
      DockLayout layout(dockspace_id, viewport->Size);
      layout.makeCentral();

      // Very simple mapping: honor dock hints if present
      ImGuiID leftId = 0, rightId = 0, upId = 0, downId = 0;
      for (auto &widget : widgets) {
        auto hint = widget->dockHint();
        if (!hint.has_value())
          continue;

        switch (hint->side) {
        case DockSide::Left:
          if (leftId == 0)
            leftId = layout.splitLeft(hint->ratio);
          layout.dockWindow(widget->name(), leftId);
          break;
        case DockSide::Right:
          if (rightId == 0)
            rightId = layout.splitRight(hint->ratio);
          layout.dockWindow(widget->name(), rightId);
          break;
        case DockSide::Up:
          if (upId == 0)
            upId = layout.splitUp(hint->ratio);
          layout.dockWindow(widget->name(), upId);
          break;
        case DockSide::Down:
          if (downId == 0)
            downId = layout.splitDown(hint->ratio);
          layout.dockWindow(widget->name(), downId);
          break;
        case DockSide::Center:
          layout.dockWindow(widget->name(), layout.centerNode());
          break;
        }
      }

      UIContext::TopBarDockId = layout.splitUp(0.05f);

      layout.finish();

      // After finishing, fetch node and set flags
      if (UIContext::TopBarDockId != 0) {
        if (ImGuiDockNode *node =
                ImGui::DockBuilderGetNode(UIContext::TopBarDockId))
          node->LocalFlags |= ImGuiDockNodeFlags_NoTabBar |
                              ImGuiDockNodeFlags_NoWindowMenuButton |
                              ImGuiDockNodeFlags_NoCloseButton;
      }
      dockBuilt = true;
    }

    // Run pre-frame hooks
    for (auto &widget : widgets)
      widget->preFrame();
  }

  void drawWidgets(){
    for (auto &widget : widgets)
      widget->draw();
  }

private:
  std::vector<std::unique_ptr<Widget>> widgets = {};

  // Layout
  DockLayout dockLayout;
  bool dockBuilt = false;
};

} // namespace Magma
