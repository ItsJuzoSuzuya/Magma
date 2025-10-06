#pragma once
#include "../../core/renderer.hpp"
#include <cstdint>

namespace Magma {

class ImGuiRenderer : public Renderer {
public:
  ImGuiRenderer(Device &device, SwapChain &swapChain,
                VkDescriptorSetLayout descriptorSetLayout);

  void getSceneSize();

  // Call at the beginning of each frame
  void newFrame();

  // Rendering
  void begin() override;
  void record() override;
  void end() override;

  // Resize
  void resize(VkExtent2D extent, VkSwapchainKHR swapChain);
};

} // namespace Magma
