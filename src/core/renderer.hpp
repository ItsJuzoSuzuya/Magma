
#pragma once
#include "device.hpp"
#include "frame_info.hpp"
#include "pipeline.hpp"
#include "render_target.hpp"
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
  // Constructor and destructor
  Renderer(Device &device, Window &window,
           VkDescriptorSetLayout descriptorSetLayout);
  ~Renderer();

  // Delete copy constructor and assignment operator
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  // Getter
  int getFrameIndex() const { return currentFrameIndex; }
  SwapChain &getSwapChain() { return *swapChain; }
  VkCommandBuffer getCurrentCommandBuffer() const {
    return commandBuffers[currentFrameIndex];
  }
  float getAspectRatio() const { return swapChain->extentAspectRatio(); }
  VkImage &getSceneImage() {
    return offscreenRenderTarget->getColorImage(currentFrameIndex);
  }
  ImTextureID getSceneTexture() { return offscreenTextures[currentFrameIndex]; }
  VkExtent2D getSceneExtent() { return offscreenRenderTarget->extent(); }

  VkCommandBuffer beginFrame();

  // Scene rendering
  void createOffscreenTextures();
  void beginScene(VkCommandBuffer commandBuffer);
  void recordScene(VkCommandBuffer commandBuffer);
  void endScene(VkCommandBuffer commandBuffer);

  // ImGui rendering
  void beginImGui(VkCommandBuffer commandBuffer);
  void recordImGui(VkCommandBuffer commandBuffer);
  void endImGui(VkCommandBuffer commandBuffer);

  // Final rendering
  void endFrame();

private:
  Device &device;
  Window &window;

  // Pipeline
  std::unique_ptr<Pipeline> swapChainPipeline;
  std::unique_ptr<Pipeline> scenePipeline;
  VkPipelineLayout pipelineLayout;
  void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
  void createPipeline(VkRenderPass renderPass);

  // Command buffers
  std::vector<VkCommandBuffer> commandBuffers;
  void createCommandBuffers();
  VkCommandBuffer imageAcquireCommandBuffer =
      device.allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  // SwapChain
  std::unique_ptr<SwapChain> swapChain;
  void recreateSwapChain();

  // Scene Rendering
  std::unique_ptr<OffscreenRenderTarget> offscreenRenderTarget;
  std::vector<ImTextureID> offscreenTextures;

  // Frame info
  uint32_t currentImageIndex;
  int currentFrameIndex = 0;
};
} // namespace Magma
