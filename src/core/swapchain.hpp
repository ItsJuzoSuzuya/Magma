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
  /** Maximum number of frames that can be processed concurrently.
   *  This value is used to create synchronization objects and manage frame rendering.
   */
  static constexpr int MAX_FRAMES_IN_FLIGHT = 2;

  SwapChain(VkExtent2D extent);
  SwapChain(VkExtent2D extent,
            std::shared_ptr<SwapChain> oldSwapChain);
  ~SwapChain();

  // --- Delete copy operations ---
  SwapChain(const SwapChain &) = delete;
  SwapChain &operator=(const SwapChain &) = delete;

  // --- Getters ----
  VkSwapchainKHR getSwapChain() const { return swapChain; }
  RenderTargetInfo getRenderInfo() const { return renderInfo; }

  // --- Rendering ---
  /** Acquires the next available image from the swap chain for rendering.
   * @return VkResult indicating the success or failure of the operation.
   */
  VkResult acquireNextImage();
  /** Submits the provided command buffer for execution and presents the rendered image.
   * @param VkCommandBuffer Pointer to be submitted.
   * @return VkResult indicating the success or failure of the operation.
   */
  VkResult submitCommandBuffer(const VkCommandBuffer *commandBuffer);

private:
  /** Information for the render target, including formats and extent. */
  RenderTargetInfo renderInfo;

  // --- Swap Chain ---
  VkSwapchainKHR swapChain;
  std::shared_ptr<SwapChain> oldSwapChain = nullptr;
  /** Creates the Vulkan swap chain with the specified extent.
   * @param extent The desired extent (width and height) for the swap chain images.
   */
  void createSwapChain(VkExtent2D &extent);

  // --- Synchronization ---
  /** Semaphore for signaling when an image has been acquired and is ready for rendering. */
  std::vector<VkSemaphore> imageAcquiredSemaphores;
  /** Semaphore for signaling when rendering is complete and the image is ready for presentation. */
  std::vector<VkSemaphore> renderCompleteSemaphores;
  /** Fences to ensure that a frame has finished rendering before starting a new one. */
  std::vector<VkFence> frameSubmitFences;
  /** Fences to track which swap chain images are currently in use. */
  std::vector<VkFence> imagesInUseFences;
  /** Creates Semaphores and Fences for synchronizing rendering and presentation. */
  void createSyncObjects();
  /** Destroys the created Semaphores and Fences to free resources. */
  void destroySyncObjects();

  // --- Helpers ---
  /** Chooses the most suitable surface format from the available options.
   * @param availableFormats A vector of available VkSurfaceFormatKHR options.
   * @return The chosen VkSurfaceFormatKHR.
   */
  VkSurfaceFormatKHR chooseSwapSurfaceFormat(
      const std::vector<VkSurfaceFormatKHR> &availableFormats);
  /** Chooses the most suitable presentation mode from the available options.
   * @param availablePresentModes A vector of available VkPresentModeKHR options.
   * @return The chosen VkPresentModeKHR.
   */
  VkPresentModeKHR chooseSwapPresentMode(
      const std::vector<VkPresentModeKHR> &availablePresentModes);
  /** Chooses the swap extent (resolution) for the swap chain images.
   * @param capabilities The VkSurfaceCapabilitiesKHR of the surface.
   * @param extent The desired extent (width and height).
   * @return The chosen VkExtent2D for the swap chain images.
   */
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                              VkExtent2D extent);
};
} // namespace Magma
