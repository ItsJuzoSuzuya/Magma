#ifndef SWAPCHAIN_HPP
#define SWAPCHAIN_HPP
#include "device.hpp"
#include <cwchar>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace magma {

class SwapChain {
public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  SwapChain(Device &device, VkExtent2D extent);
  SwapChain(Device &device, VkExtent2D extent,
            std::shared_ptr<SwapChain> oldSwapChain);
  ~SwapChain();

  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

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

  VkResult acquireNextImage(uint32_t *imageIndex);
  VkResult submitCommandBuffer(const VkCommandBuffer *commandBuffer,
                               uint32_t *imageIndex);

  bool compareSwapFormats(const SwapChain &swapChain) {
    return swapChain.swapChainImageFormat == swapChainImageFormat &&
           swapChain.swapChainDepthFormat == swapChainDepthFormat;
  }

private:
  Device &device;
  VkExtent2D windowExtent;

  VkSwapchainKHR swapChain;
  VkFormat swapChainImageFormat;
  VkFormat swapChainDepthFormat;
  VkExtent2D swapChainExtent;
  std::vector<VkImage> swapChainImages;
  std::vector<VkImageView> swapChainImageViews;
  std::shared_ptr<SwapChain> oldSwapChain;

  VkRenderPass renderPass;

  std::vector<VkImage> depthImages;
  std::vector<VkDeviceMemory> depthImageMemories;
  std::vector<VkImageView> depthImageViews;
  std::vector<VkImageLayout> depthImageLayouts;

  std::vector<VkFramebuffer> framebuffers;

  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  size_t currentFrame = 0;

  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createDepthResources();
  void createFramebuffers();
  void createSyncObjects();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);
  VkFormat findDepthFormat();
};
} // namespace magma
#endif
