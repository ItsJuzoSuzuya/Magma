#pragma once
#include "imgui_impl_vulkan.h"
#include "queue_family_indices.hpp"
#include <cstdint>
#include <vector>
#include <vulkan/vk_platform.h>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Window;

struct SwapchainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

class Device {
public:
  Device(Window &window);
  ~Device();

  Device(const Device &) = delete;
  Device &operator=(const Device &) = delete;
  Device(const Device &&) = delete;
  Device &operator=(const Device &&) = delete;

  static Device &get() { return *instance_; }
  static VkDeviceSize nonCoherentAtomSize() {
    return get().properties.limits.nonCoherentAtomSize;
  }
  VkDevice device() { return device_; }
  VkSurfaceKHR surface() { return surface_; }
  VkCommandPool getCommandPool() { return commandPool; }
  VkQueue graphicsQueue() { return graphicsQueue_; }
  VkQueue presentQueue() { return presentQueue_; }

  void populateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *init_info);

  SwapchainSupportDetails getSwapChainSupport() {
    return querySwapChainSupport(physicalDevice);
  }
  QueueFamilyIndices findQueueFamilies() {
    return findQueueFamilies(physicalDevice);
  }

  void createImageWithInfo(const VkImageCreateInfo &imageInfo,
                           VkMemoryPropertyFlags properties, VkImage &image,
                           VkDeviceMemory &imageMemory);
  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);

  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                  VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);
  void copyImageToBuffer(VkCommandBuffer &commandBuffer, VkBuffer dstBuffer,
                         VkImage image, VkBufferImageCopy region);

  static void transitionImageLayout(
      VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
      VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
      VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
      VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

  VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level);
  VkCommandBuffer beginSingleTimeCommands();
  void submitCommands(VkCommandBuffer &commandBuffer);
  void endSingleTimeCommands(VkCommandBuffer &commandBuffer);

  VkFormat findDepthFormat();

  static void waitIdle() { vkDeviceWaitIdle(get().device_); }

private:
  #ifdef NDEBUG
    const bool enableValidationLayers = false;
  #else
    const bool enableValidationLayers = true;
    const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
  #endif
  bool checkValidationLayerSupport();

  inline static Device *instance_ = nullptr;

  VkInstance instance;
  void createInstance();
  std::vector<const char *> getRequiredExtensions();

  VkDebugUtilsMessengerEXT debugMessenger;
  void setupDebugMessenger();

  VkSurfaceKHR surface_;

  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties properties;
  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MULTI_DRAW_EXTENSION_NAME,
      VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME};
  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);

  VkDevice device_;
  VkQueue graphicsQueue_;
  VkQueue presentQueue_;
  void createLogicalDevice();

  VkCommandPool commandPool;
  void createCommandPool();

  VkFence fence;
  void createFence();

  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
};
} // namespace Magma
