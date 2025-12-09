#include "swapchain.hpp"
#include "device.hpp"
#include "frame_info.hpp"
#include "queue_family_indices.hpp"
#include "render_system.hpp"
#include "render_target_info.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <print>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
SwapChain::SwapChain(VkExtent2D extent) {
  createSwapChain(extent);
  createSyncObjects();
}

SwapChain::SwapChain(VkExtent2D extent, std::shared_ptr<SwapChain> oldSwapChain)
    : oldSwapChain{oldSwapChain} {
  createSwapChain(extent);
  createSyncObjects();
  this->oldSwapChain = nullptr;
}

// Destructor
SwapChain::~SwapChain() {
  destroySyncObjects();
  VkDevice device = Device::get().device();
  vkDestroySwapchainKHR(device, swapChain, nullptr);
}

// --- Public ---
// Rendering
VkResult SwapChain::acquireNextImage() {
  VkDevice device = Device::get().device();

  // Wait for the fence for the current frame (we are going to reuse its command
  // buffer)
  vkWaitForFences(device, 1, &inFlightFences[FrameInfo::frameIndex], VK_TRUE,
                  UINT64_MAX);

  // Acquire next image using per-frame semaphore
  VkResult result =
      vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                            imageAvailableSemaphores[FrameInfo::frameIndex],
                            VK_NULL_HANDLE, &FrameInfo::imageIndex);

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    return result;

  if (renderInfo.imageCount == 0)
    return VK_ERROR_OUT_OF_DATE_KHR;

  if (FrameInfo::imageIndex >= renderInfo.imageCount)
    return VK_ERROR_OUT_OF_DATE_KHR;

  // If that image is already in flight, wait for its fence
  if (imagesInFlight[FrameInfo::imageIndex] != VK_NULL_HANDLE) {
    vkWaitForFences(device, 1, &imagesInFlight[FrameInfo::imageIndex], VK_TRUE,
                    UINT64_MAX);
  }

  return result;
}

VkResult SwapChain::submitCommandBuffer(const VkCommandBuffer *commandBuffer) {
  Device &device = Device::get();

  // Associate the image with the fence of the current frame
  imagesInFlight[FrameInfo::imageIndex] = inFlightFences[FrameInfo::frameIndex];

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

  VkSemaphore waitSemaphores[] = {
      imageAvailableSemaphores[FrameInfo::frameIndex]}; 

  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = commandBuffer;

  VkSemaphore signalSemaphores[] = {
      renderFinishedSemaphores[FrameInfo::imageIndex]}; 

  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(device.device(), 1, &inFlightFences[FrameInfo::frameIndex]);
  if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo,
                    inFlightFences[FrameInfo::frameIndex]) != VK_SUCCESS)
    throw std::runtime_error("Failed to submit draw command buffer!");

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  presentInfo.pWaitSemaphores = signalSemaphores;

  VkSwapchainKHR swapChains[] = {swapChain};
  presentInfo.swapchainCount = 1;
  presentInfo.pSwapchains = swapChains;
  presentInfo.pImageIndices = &FrameInfo::imageIndex;

  return vkQueuePresentKHR(device.presentQueue(), &presentInfo);
}

// --- Private ---
// Swap Chain
void SwapChain::createSwapChain(VkExtent2D &extent) {
  Device &device = Device::get();
  SwapchainSupportDetails swapChainSupport = device.getSwapChainSupport();

  VkSurfaceFormatKHR surfaceFormat =
      chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode =
      chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D chosenExtent =
      chooseSwapExtent(swapChainSupport.capabilities, extent);

  uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
  if (swapChainSupport.capabilities.maxImageCount > 0 &&
      imageCount > swapChainSupport.capabilities.maxImageCount)
    imageCount = swapChainSupport.capabilities.maxImageCount;

  VkSwapchainCreateInfoKHR createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
  createInfo.surface = device.surface();
  createInfo.minImageCount = imageCount;
  createInfo.imageFormat = surfaceFormat.format;
  createInfo.imageColorSpace = surfaceFormat.colorSpace;
  createInfo.imageExtent = chosenExtent;
  createInfo.imageArrayLayers = 1;
  createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

  QueueFamilyIndices indices = device.findQueueFamilies();
  if (indices.presentFamily.value() != indices.graphicsFamily.value()) {
    createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
    createInfo.queueFamilyIndexCount = 2;
    uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(),
                                     indices.presentFamily.value()};
    createInfo.pQueueFamilyIndices = queueFamilyIndices;
  } else {
    createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
  }

  createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
  createInfo.presentMode = presentMode;
  createInfo.clipped = VK_TRUE;
  createInfo.oldSwapchain =
      oldSwapChain == nullptr ? VK_NULL_HANDLE : oldSwapChain->swapChain;
  createInfo.preTransform = swapChainSupport.capabilities.currentTransform;

  if (vkCreateSwapchainKHR(device.device(), &createInfo, nullptr, &swapChain) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create swap chain!");

  uint32_t actualImageCount = 0;
  if (vkGetSwapchainImagesKHR(device.device(), swapChain, &actualImageCount,
                              nullptr) != VK_SUCCESS)
    throw std::runtime_error("Failed to get swapchain image count!");

  if (actualImageCount == 0)
    throw std::runtime_error("Created swapchain has 0 images!");

  renderInfo = {
      .extent = chosenExtent,
      .colorFormat = surfaceFormat.format,
      .depthFormat = device.findDepthFormat(),
      .imageCount = actualImageCount,
  };
}

// Synchronization
void SwapChain::createSyncObjects() {
  VkDevice device = Device::get().device();

  imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
  // Per-image render-finished semaphores sized to swapchain image count
  renderFinishedSemaphores.resize(renderInfo.imageCount);
  inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
  imagesInFlight.resize(renderInfo.imageCount, VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < imageAvailableSemaphores.size(); i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAvailableSemaphores[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create imageAvailable semaphore!");
  }

  for (size_t i = 0; i < renderFinishedSemaphores.size(); i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderFinishedSemaphores[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create renderFinished semaphore!");
  }

  for (size_t i = 0; i < inFlightFences.size(); i++) {
    if (vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) !=
        VK_SUCCESS)
      throw std::runtime_error("Failed to create inFlight fence!");
  }
}

void SwapChain::destroySyncObjects() {
  VkDevice device = Device::get().device();

  for (auto s : imageAvailableSemaphores)
    vkDestroySemaphore(device, s, nullptr);
  for (auto s : renderFinishedSemaphores)
    vkDestroySemaphore(device, s, nullptr);
  for (auto f : inFlightFences)
    vkDestroyFence(device, f, nullptr);

  imageAvailableSemaphores.clear();
  renderFinishedSemaphores.clear();
  inFlightFences.clear();
  imagesInFlight.clear();
}

// Helpers (unchanged)
VkSurfaceFormatKHR SwapChain::chooseSwapSurfaceFormat(
    const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR)
      return availableFormat;
  }
  return availableFormats[0];
}

VkPresentModeKHR SwapChain::chooseSwapPresentMode(
    const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR)
      return availablePresentMode;
  }
  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D
SwapChain::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities,
                            VkExtent2D extent) {
  if (capabilities.currentExtent.width != UINT32_MAX) {
    return capabilities.currentExtent;
  } else {
    VkExtent2D actualExtent = extent;
    actualExtent.width =
        std::clamp(actualExtent.width, capabilities.minImageExtent.width,
                   capabilities.maxImageExtent.width);
    actualExtent.height =
        std::clamp(actualExtent.height, capabilities.minImageExtent.height,
                   capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

} // namespace Magma
