
#pragma once

#include "magma_device.hpp"
#include "magma_swap_chain.hpp"
#include <cassert>
#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace magma {
class MagmaRenderer {
public:
  MagmaRenderer(MagmaWindow &window, MagmaDevice &device);
  ~MagmaRenderer();

  MagmaRenderer(const MagmaRenderer &) = delete;
  MagmaRenderer &operator=(const MagmaRenderer &) = delete;

  VkRenderPass getSwapChainRenderPass() const {
    return magmaSwapChain->getRenderPass();
  }
  bool isFrameInProgress() const { return isFrameStarted; }

  VkCommandBuffer getCurrentCommandBuffer() const {
    assert(isFrameStarted &&
           "Cannot get command buffer when frame not in progress!");
    return commandBuffers[currentImageIndex];
  }

  VkCommandBuffer beginFrame();
  void endFrame();
  void beginSwapChainRenderPass(VkCommandBuffer commandBuffer);
  void endSwapChainRenderPass(VkCommandBuffer commandBuffer);

private:
  void createCommandBuffers();
  void freeCommandBuffers();
  void recreateSwapChain();

  MagmaWindow &magmaWindow;
  MagmaDevice &magmaDevice;
  std::unique_ptr<MagmaSwapChain> magmaSwapChain;
  std::vector<VkCommandBuffer> commandBuffers;

  uint32_t currentImageIndex;
  bool isFrameStarted = false;
};

} // namespace magma
