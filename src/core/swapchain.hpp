#pragma once
#include "render_target_info.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

struct RenderTargetInfo;

class SwapChain {
public:
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  SwapChain(VkExtent2D extent);
  SwapChain(VkExtent2D extent,
            std::shared_ptr<SwapChain> oldSwapChain);
  ~SwapChain();

  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

  // Getters
  VkSwapchainKHR getSwapChain() const { return swapChain; }
  VkFence &getInFlightFence(int index) { return inFlightFences[index]; }
  RenderTargetInfo getRenderInfo() const { return renderInfo; }

  // Rendering
  VkResult acquireNextImage();
  VkResult submitCommandBuffer(const VkCommandBuffer *commandBuffer);

private:
  RenderTargetInfo renderInfo;

  // Swap Chain
  VkSwapchainKHR swapChain;
  std::shared_ptr<SwapChain> oldSwapChain = nullptr;
  void createSwapChain(VkExtent2D &extent);

  // Synchronization
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;
  std::vector<VkFence> imagesInFlight;
  void createSyncObjects();
  void destroySyncObjects();

  // Helpers
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                              VkExtent2D extent);
};
} // namespace Magma
