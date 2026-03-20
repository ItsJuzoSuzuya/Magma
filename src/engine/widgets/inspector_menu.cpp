module;
#include "imgui.h"

module widgets:inspector_menu;

namespace Magma {

/**
 * Inspector Menu Widget
 */
export class InspectorMenu : public Widget {
public:
  const char *name() const override { return "Inspector Menu"; }

  /**
   * Queue opening Menu 
   */
  static void queueContextMenuFor(GameObject *target) {
    contextTarget = target;
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
      if (contextTarget) {
        ImGui::TextUnformatted(contextTarget->name.c_str());
        ImGui::Separator();
        drawAddComponentMenu();
      } else {
        ImGui::TextUnformatted("Scene");
        ImGui::Separator();
        if (ImGui::MenuItem("Add Entity"))
          GameObject::create();
        if (ImGui::MenuItem("Delete"))
          ; // No scene deletion for now
      }

      ImGui::EndPopup();
    }
  }

private:
  inline static GameObject *contextTarget = nullptr;
  inline static bool openPopupRequested = false;

  void drawAddComponentMenu() {
    if (!contextTarget)
      return;

    if (ImGui::BeginMenu("Add Component")) {
      if (ImGui::MenuItem("Transform"))
        contextTarget->addComponent<Transform>();
      if (ImGui::MenuItem("Mesh"))
        contextTarget->addComponent<Mesh>();
      if (ImGui::MenuItem("Point Light"))
        contextTarget->addComponent<PointLight>();
      ImGui::EndMenu();
    }
  }
};
} // namespace Magma
