module;
#include "imgui.h"

export module widgets:scene_tree;
import :widget;
import engine:scene;

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

    for (auto &gameObject : Scene::current()->getGameObjects()) {
      if (!gameObject) continue;

      #if defined(MAGMA_WITH_EDITOR)
        gameObject->editorCallbacks = {
            .onLeftClick  = [](GameObject* g){ Inspector::setContext(g); },
            .onRightClick = [](GameObject* g){ SceneMenu::queueContextMenuFor(g); }
        };
      #endif

            //SceneMenu::queueContextMenuFor(gameObject.get());

      Scene::drawSceneTree();
      sceneMenu.draw();
    }

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
