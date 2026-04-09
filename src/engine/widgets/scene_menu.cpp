module;
#include "imgui.h"
#include <memory>

export module widgets:scene_menu;
import :widget;
import :ui_context;
import :inspector;
import engine;
import components;

namespace Magma {

export class SceneMenu : public Widget {
public:
  const char *name() const override { return "Scene Menu"; }

  // Getters
  static GameObject *getContextTarget() { return contextTarget; }

  // Setters
  static void setContextTarget(GameObject *gameObject) {
    contextTarget = gameObject;
  }

  // Queue opening the context menu at window-root scope this frame
  static void queueContextMenuFor(GameObject *gameObject) {
    contextTarget = gameObject;
    openPopupRequested = true;
  }

  // Render
  void draw() override {
    if (openPopupRequested) {
      ImGui::OpenPopup(name());
      openPopupRequested = false;
    }

    // Popup menu
    if (ImGui::BeginPopup(name())) {
      if (auto *target = getContextTarget()) {
        ImGui::TextUnformatted(target->name.c_str());
        ImGui::Separator();
        if (ImGui::MenuItem("Add Child"))
          target->addChild();
        if (ImGui::MenuItem("Delete")){
          Scene::current()->removeGameObject(target);

          #if defined(MAGMA_WITH_EDITOR)
            Inspector::setContext(nullptr);
          #endif
        }
      } else {
        ImGui::TextUnformatted("Scene");
        ImGui::Separator();
        if (ImGui::MenuItem("Add Entity")) {
          if (Scene::current())
            Scene::current()->addGameObject(std::make_unique<GameObject>(0));
        }
        if (ImGui::MenuItem("Add Camera")) {
          if (Scene::current()) {
            auto camObj = std::make_unique<GameObject>(0, "Camera");
            camObj->addComponent<Transform>();
            camObj->addComponent<Camera>();
            Scene::current()->addGameObject(std::move(camObj));
          }
        }
      }

      ImGui::EndPopup();
    }
  }

private:
  inline static GameObject *contextTarget = nullptr;
  inline static bool openPopupRequested = false;
};
} // namespace Magma
