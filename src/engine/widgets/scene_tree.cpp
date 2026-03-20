module;
#include "imgui.h"

module widgets:scene_tree;
import std;

namespace Magma {

export class SceneTree : public Widget {
public:
  // Name of the widget
  const char *name() const override { return "Scene Tree"; }

  // Lifecycle
  void preFrame() override {
    UIContext::ensureInit();
    ImGui::SetNextWindowClass(&UIContext::AppDockClass);
    ImGui::Begin(name());
    ImGui::End();
  }

  void draw() override {
    UIContext::ensureInit();

    ImGui::SetNextWindowClass(&UIContext::AppDockClass);
    ImGui::Begin(name());

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Right) && ImGui::IsWindowHovered()) 
      SceneMenu::queueContextMenuFor(nullptr);

    Scene::drawSceneTree();
    sceneMenu.draw();

    ImGui::End();
  }


  // Docking prefrence (Left 25%)
  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Left, 0.25f};
  }

private:
  // Scene Menu
  SceneMenu sceneMenu = {};
};

} // namespace Magma
