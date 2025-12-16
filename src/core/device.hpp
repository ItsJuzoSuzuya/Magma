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

  // Getters
  static Device &get() { return *instance_; }
  static VkDeviceSize nonCoherentAtomSize() {
    return get().properties.limits.nonCoherentAtomSize;
  }
  VkDevice device() { return device_; }
  VkSurfaceKHR surface() { return surface_; }
  VkCommandPool getCommandPool() { return commandPool; }
  VkQueue graphicsQueue() { return graphicsQueue_; }
  VkQueue presentQueue() { return presentQueue_; }

  // ImGui Init Info Population
  void populateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *init_info);

  // Swapchain Support
  SwapchainSupportDetails getSwapChainSupport() {
    return querySwapChainSupport(physicalDevice);
  }

  // Queue Families
  QueueFamilyIndices findQueueFamilies() {
    return findQueueFamilies(physicalDevice);
  }

  // Image Creation
  void createImageWithInfo(const VkImageCreateInfo &imageInfo,
                           VkMemoryPropertyFlags properties, VkImage &image,
                           VkDeviceMemory &imageMemory);
  VkFormat findSupportedFormat(const std::vector<VkFormat> &candidates,
                               VkImageTiling tiling,
                               VkFormatFeatureFlags features);

  // Buffer
  void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                    VkMemoryPropertyFlags properties, VkBuffer &buffer,
                    VkDeviceMemory &bufferMemory);
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size,
                  VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
  void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                         uint32_t height);
  void copyImageToBuffer(VkCommandBuffer &commandBuffer, VkBuffer dstBuffer,
                         VkImage image, VkBufferImageCopy region);

  // Image Layout Transition
  static void transitionImageLayout(
      VkImage image, VkImageLayout oldLayout, VkImageLayout newLayout,
      VkPipelineStageFlags srcStage, VkPipelineStageFlags dstStage,
      VkAccessFlags srcAccessMask, VkAccessFlags dstAccessMask,
      VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT);

  // Command Buffers
  VkCommandBuffer allocateCommandBuffer(VkCommandBufferLevel level);
  VkCommandBuffer beginSingleTimeCommands();
  void submitCommands(VkCommandBuffer &commandBuffer);
  void endSingleTimeCommands(VkCommandBuffer &commandBuffer);

  // Depth Format
  VkFormat findDepthFormat();

  // Sync
  static void waitIdle() { vkDeviceWaitIdle(get().device_); }

private:
  // Validation Layers
#ifdef NDEBUG
  const bool enableValidationLayers = false;
#else
  const bool enableValidationLayers = true;
#endif
  const std::vector<const char *> validationLayers = {
      "VK_LAYER_KHRONOS_validation"};
  bool checkValidationLayerSupport();

  inline static Device *instance_ = nullptr;

  // Instance
  VkInstance instance;
  void createInstance();
  std::vector<const char *> getRequiredExtensions();

  // Debug Messenger
  VkDebugUtilsMessengerEXT debugMessenger;
  void setupDebugMessenger();

  // Surface
  VkSurfaceKHR surface_;

  // Physical Device
  VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
  VkPhysicalDeviceProperties properties;
  void pickPhysicalDevice();
  bool isDeviceSuitable(VkPhysicalDevice device);
  SwapchainSupportDetails querySwapChainSupport(VkPhysicalDevice device);

  // Device Extensions
  const std::vector<const char *> deviceExtensions = {
      VK_KHR_SWAPCHAIN_EXTENSION_NAME, VK_EXT_MULTI_DRAW_EXTENSION_NAME,
      VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME};
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  // Logical Device
  VkDevice device_;
  VkQueue graphicsQueue_;
  VkQueue presentQueue_;
  void createLogicalDevice();

  // Command Pool
  VkCommandPool commandPool;
  void createCommandPool();

  // Fence
  VkFence fence;
  void createFence();

  // Queue Families
  QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);

  // Memory Management
  uint32_t findMemoryType(uint32_t typeFilter,
                          VkMemoryPropertyFlags properties);
};
} // namespace Magma
