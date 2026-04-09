module;
#include <optional>
#include <vector>
#include "imgui.h"

export module widgets:scene_tree;
import :widget;
import :ui_context;
import :scene_menu;
import :inspector;
import engine;

namespace Magma {

export class SceneTree : public Widget {
public:
  const char *name() const override { return "Scene Tree"; }

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

    if (Scene::current()) {
      for (auto &gameObject : Scene::current()->getGameObjects()) {
        if (!gameObject) continue;

        #if defined(MAGMA_WITH_EDITOR)
          gameObject->callbacks = {
              .onLeftClick  = [](GameObject* g){ Inspector::setContext(g); },
              .onRightClick = [](GameObject* g){ SceneMenu::queueContextMenuFor(g); }
          };
        #endif

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;
        if (gameObject->getChildren().empty())
          flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;

        bool open = ImGui::TreeNodeEx(gameObject->name.c_str(), flags);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) &&
            gameObject->callbacks.onRightClick)
          gameObject->callbacks.onRightClick(gameObject.get());
        if (ImGui::IsItemClicked() && gameObject->callbacks.onLeftClick)
          gameObject->callbacks.onLeftClick(gameObject.get());

        if (open && !gameObject->getChildren().empty()) {
          gameObject->drawChildren();
          ImGui::TreePop();
        }
      }
    }

    sceneMenu.draw();

    ImGui::End();
  }

  std::optional<DockHint> dockHint() const override {
    return DockHint{DockSide::Left, 0.25f};
  }

private:
  SceneMenu sceneMenu = {};
};

} // namespace Magma
