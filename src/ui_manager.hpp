#include "engine/editor_camera.hpp"
#include "engine/gameobject.hpp"
#include "engine/render/imgui_renderer.hpp"
#include "engine/widgets/wiget_manger.hpp"
#include <functional>
#include <memory>
#include <print>
#include <utility>
#include <vulkan/vulkan_core.h>
#include <glm/trigonometric.hpp>


namespace Magma {

class UI_Manager {
public:
  UI_Manager(){}

  std::unique_ptr<ImGuiRenderer> setupUI(Window *window, WidgetManager *widgetManager);

  std::unique_ptr<SceneRenderer> moveEditorRenderer(){
    return std::move(editorRenderer);
  }

  std::unique_ptr<SceneRenderer> moveGameRenderer(){
    return std::move(gameRenderer);
  }

private:
  std::unique_ptr<SceneRenderer> editorRenderer;
  std::unique_ptr<SceneRenderer> gameRenderer;
  std::unique_ptr<GameObject> editorCameraObject;
  EditorCamera editorCamera;

  std::unique_ptr<SceneRenderer> createEditorRenderer();
  std::unique_ptr<SceneRenderer> createGameRenderer();
};

} // namespace Magma
