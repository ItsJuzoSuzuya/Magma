#pragma once
#include <cwchar>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Device;

class SwapChain {
public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  SwapChain(Device &device, VkExtent2D extent);
  SwapChain(Device &device, VkExtent2D extent,
            std::shared_ptr<SwapChain> oldSwapChain);
  ~SwapChain();

  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

  // Getters
  VkRenderPass getRenderPass() { return renderPass; }
  VkFramebuffer getFrameBuffer(int index) { return framebuffers[index]; }
  VkImage &getDepthImage(int index) { return depthImages[index]; }
  VkExtent2D &extent() { return swapChainExtent; }
  float extentAspectRatio() {
    return static_cast<float>(swapChainExtent.width) /
           static_cast<float>(swapChainExtent.height);
  }
  VkImageLayout &getImageLayout(int index) { return depthImageLayouts[index]; }
  VkFence &getInFlightFence(int index) { return inFlightFences[index]; }
  VkFormat getImageFormat() const { return swapChainImageFormat; }
  VkFormat getDepthFormat() const { return swapChainDepthFormat; }
  uint32_t imageCount() const {
    return static_cast<uint32_t>(swapChainImages.size());
  }

  // Rendering
  VkResult acquireNextImage(uint32_t *imageIndex);
  VkResult submitCommandBuffer(const VkCommandBuffer *commandBuffer,
                               uint32_t *imageIndex);

  // Comparison
  bool compareSwapFormats(const SwapChain &swapChain) {
    return swapChain.swapChainImageFormat == swapChainImageFormat &&
           swapChain.swapChainDepthFormat == swapChainDepthFormat;
  }

private:
  Device &device;
  VkExtent2D windowExtent;

  // Swap Chain
  VkSwapchainKHR swapChain;
  std::shared_ptr<SwapChain> oldSwapChain;
  VkExtent2D swapChainExtent;
  void createSwapChain();

  // Swap Chain Images
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  VkFormat swapChainImageFormat;
  void createImages(uint32_t imagecount);
  void createImageViews();

  // Render Pass
  VkRenderPass renderPass;
  void createRenderPass();

  // Depth Resources
  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  std::vector<VkImageLayout> depthImageLayouts;
  VkFormat swapChainDepthFormat;
  void createDepthResources();

  // Framebuffers
  std::vector<VkFramebuffer> framebuffers;
  void createFramebuffers();

  // Synchronization

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  size_t currentFrame = 0;
  void createSyncObjects();

  // Helpers
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  VkFormat findDepthFormat();
};
} // namespace Magma
