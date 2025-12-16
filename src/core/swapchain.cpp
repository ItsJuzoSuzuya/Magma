#include "swapchain.hpp"
#include "device.hpp"
#include "frame_info.hpp"
#include "queue_family_indices.hpp"
#include "render_target_info.hpp"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <print>
#include <stdexcept>
#include <vector>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

// Constructor
SwapChain::SwapChain(VkExtent2D extent) {
  createSwapChain(extent);

  println("Swapchain details:");
  println("  Image Count: {}", renderInfo.imageCount);
  println("  Color Format: {}", string_VkFormat(renderInfo.colorFormat));
  println("  Depth Format: {}", string_VkFormat(renderInfo.depthFormat));
  println("  Extent: {}x{}", renderInfo.extent.width, renderInfo.extent.height);
  println("  Frames In Flight: {}", MAX_FRAMES_IN_FLIGHT);

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

  // Wait for the previous frame to finish
  vkWaitForFences(device, 1, &frameSubmitFences[FrameInfo::frameIndex], VK_TRUE,
                  UINT64_MAX);

  VkResult result =
      vkAcquireNextImageKHR(device, swapChain, UINT64_MAX,
                            imageAcquiredSemaphores[FrameInfo::frameIndex],
                            VK_NULL_HANDLE, &FrameInfo::imageIndex);

  if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    return result;

  if (renderInfo.imageCount == 0)
    return VK_ERROR_OUT_OF_DATE_KHR;

  if (FrameInfo::imageIndex >= renderInfo.imageCount)
    return VK_ERROR_OUT_OF_DATE_KHR;

  // If that image is still in use, wait for its fence
  if (imagesInUseFences[FrameInfo::imageIndex] != VK_NULL_HANDLE)
    vkWaitForFences(device, 1, &imagesInUseFences[FrameInfo::imageIndex],
                    VK_TRUE, UINT64_MAX);

  return result;
}

VkResult SwapChain::submitCommandBuffer(const VkCommandBuffer *commandBuffer) {
  Device &device = Device::get();

  // Associate the image with the fence of the current frame
  imagesInUseFences[FrameInfo::imageIndex] =
      frameSubmitFences[FrameInfo::frameIndex];

  VkSubmitInfo submitInfo = {};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = commandBuffer;

  // Wait for the image to be acquired submit
  VkSemaphore waitSemaphores[] = {
      imageAcquiredSemaphores[FrameInfo::frameIndex]};
  // Wait in the color attachment output stage
  VkPipelineStageFlags waitStages[] = {
      VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
  submitInfo.waitSemaphoreCount = 1;
  submitInfo.pWaitSemaphores = waitSemaphores;
  submitInfo.pWaitDstStageMask = waitStages;

  // Signal that rendering is complete
  VkSemaphore signalSemaphores[] = {
      renderCompleteSemaphores[FrameInfo::imageIndex]};
  submitInfo.signalSemaphoreCount = 1;
  submitInfo.pSignalSemaphores = signalSemaphores;

  vkResetFences(device.device(), 1, &frameSubmitFences[FrameInfo::frameIndex]);
  if (vkQueueSubmit(device.graphicsQueue(), 1, &submitInfo,
                    frameSubmitFences[FrameInfo::frameIndex]) != VK_SUCCESS)
    throw std::runtime_error("Failed to submit draw command buffer!");

  VkPresentInfoKHR presentInfo = {};
  presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
  presentInfo.waitSemaphoreCount = 1;
  // Present after signal that rendering is complete
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

  imageAcquiredSemaphores.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
  renderCompleteSemaphores.resize(renderInfo.imageCount, VK_NULL_HANDLE);
  frameSubmitFences.resize(MAX_FRAMES_IN_FLIGHT, VK_NULL_HANDLE);
  imagesInUseFences.resize(renderInfo.imageCount, VK_NULL_HANDLE);

  VkSemaphoreCreateInfo semaphoreInfo = {};
  semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

  VkFenceCreateInfo fenceInfo = {};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
  fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

  for (size_t i = 0; i < imageAcquiredSemaphores.size(); i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &imageAcquiredSemaphores[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create imageAvailable semaphore!");
  }

  for (size_t i = 0; i < renderCompleteSemaphores.size(); i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr,
                          &renderCompleteSemaphores[i]) != VK_SUCCESS)
      throw std::runtime_error("Failed to create renderFinished semaphore!");
  }

  for (size_t i = 0; i < frameSubmitFences.size(); i++) {
    if (vkCreateFence(device, &fenceInfo, nullptr, &frameSubmitFences[i]) !=
        VK_SUCCESS)
      throw std::runtime_error("Failed to create inFlight fence!");
  }
}

void SwapChain::destroySyncObjects() {
  VkDevice device = Device::get().device();

  for (auto s : imageAcquiredSemaphores)
    vkDestroySemaphore(device, s, nullptr);
  for (auto s : renderCompleteSemaphores)
    vkDestroySemaphore(device, s, nullptr);
  for (auto f : frameSubmitFences)
    vkDestroyFence(device, f, nullptr);

  imageAcquiredSemaphores.clear();
  renderCompleteSemaphores.clear();
  frameSubmitFences.clear();
  imagesInUseFences.clear();
}

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
        clamp(actualExtent.width, capabilities.minImageExtent.width,
              capabilities.maxImageExtent.width);
    actualExtent.height =
        clamp(actualExtent.height, capabilities.minImageExtent.height,
              capabilities.maxImageExtent.height);

    return actualExtent;
  }
}

} // namespace Magma
