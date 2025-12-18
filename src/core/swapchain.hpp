#pragma once
#include "render_target_info.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

struct RenderTargetInfo;

/** SwapChain class manages the Vulkan swap chain, including image acquisition
 *  and presentation, as well as synchronization objects for rendering and presentation.
 */
class SwapChain {
public:
  /* Maximum number of frames that can be processed concurrently.*/
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  SwapChain(VkExtent2D extent);
  SwapChain(VkExtent2D extent,
            std::shared_ptr<SwapChain> oldSwapChain);
  ~SwapChain();

  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

  VkSwapchainKHR getSwapChain() const { return swapChain; }
  RenderTargetInfo getRenderInfo() const { return renderInfo; }

  VkResult acquireNextImage();
  VkResult submitCommandBuffer(const VkCommandBuffer *commandBuffer);

private:
  RenderTargetInfo renderInfo;

  VkSwapchainKHR swapChain;
  std::shared_ptr<SwapChain> oldSwapChain = nullptr;
  void createSwapChain(VkExtent2D &extent);

  /** Semaphore for signaling when an image has been acquired and is ready for rendering. */
  std::vector<VkSemaphore> imageAcquiredSemaphores;
  /** Semaphore for signaling when rendering is complete and the image is ready for presentation. */
  std::vector<VkSemaphore> renderCompleteSemaphores;
  /** Fences to ensure that a frame has finished rendering before starting a new one. */
  std::vector<VkFence> frameSubmitFences;
  /** Fences to track which swap chain images are currently in use. */
  std::vector<VkFence> imagesInUseFences;
  void createSyncObjects();
  void destroySyncObjects();

  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                              VkExtent2D extent);
};
} // namespace Magma
