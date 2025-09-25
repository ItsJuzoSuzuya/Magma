
#pragma once
#include "device.hpp"
#include "frame_info.hpp"
#include "pipeline.hpp"
#include "swapchain.hpp"
#include <cstdint>
#include <memory>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan_core.h>
namespace Magma {

class Window;

class Renderer {
public:
  Renderer(Device &device, Window &window,
           VkDescriptorSetLayout descriptorSetLayout);
  ~Renderer();

  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  int getFrameIndex() const { return currentFrameIndex; }
  SwapChain &getSwapChain() { return *swapChain; }

  VkCommandBuffer beginFrame();
  VkCommandBuffer beginDepthImageAcquire();
  VkCommandBuffer getCurrentCommandBuffer() const {
    return commandBuffers[currentFrameIndex];
  }
  float getAspectRatio() const { return swapChain->extentAspectRatio(); }
  void recordCommandBuffer(VkCommandBuffer commandBuffer);
  void endRenderPass(VkCommandBuffer commandBuffer);
  void endFrame();

private:
  Device &device;
  Window &window;
  VkPipelineLayout pipelineLayout;
  std::unique_ptr<Pipeline> pipeline;
  std::unique_ptr<SwapChain> swapChain;
  std::vector<VkCommandBuffer> commandBuffers;

  VkCommandBuffer imageAcquireCommandBuffer =
      device.allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
  void createPipeline(VkRenderPass renderPass);
  void createCommandBuffers();

  void recreateSwapChain();

  uint32_t currentImageIndex;
  int currentFrameIndex = 0;
};
} // namespace Magma
