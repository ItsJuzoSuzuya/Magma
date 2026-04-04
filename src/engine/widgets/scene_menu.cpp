module;
#include "imgui.h"

module widgets:scene_menu;
import :widget;

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
  void SceneMenu::draw() override {
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
        if (ImGui::MenuItem("Add Entity"))
          GameObject::create();
        if (ImGui::MenuItem("Add Camera")) {
          auto &obj = GameObject::create("Camera");
          obj.addComponent<Transform>();
          obj.addComponent<Camera>();
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
