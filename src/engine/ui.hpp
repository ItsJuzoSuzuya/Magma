#pragma once
#include "engine/render/imgui_renderer.hpp"
#include "engine/widgets/wiget_manger.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>
#include <glm/trigonometric.hpp>


namespace Magma {

class UI {
public:
  static std::unique_ptr<ImGuiRenderer> setup(Window *window, WidgetManager *widgetManager);

private:
};

} // namespace Magma
