#pragma once
#include "../../core/renderer.hpp"
#include <cstdint>

namespace Magma {

class ImGuiRenderer : public Renderer {
public:
  ImGuiRenderer(Device &device, SwapChain &swapChain,
                VkDescriptorSetLayout descriptorSetLayout);

  void getSceneSize();

  void begin(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;
  void record(VkCommandBuffer commandBuffer) override;
  void end(VkCommandBuffer commandBuffer) override;
};

} // namespace Magma
