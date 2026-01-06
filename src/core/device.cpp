#include "device.hpp"
#include "external/stb_image.h"
#include "frame_info.hpp"
#include "window.hpp"
#include <GLFW/glfw3.h>
#include <X11/X.h>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <ostream>
#include <print>
#include <set>
#include <vulkan/vk_enum_string_helper.h>
#include <vulkan/vulkan_core.h>


namespace DebugMessenger {

static VKAPI_ATTR VkBool32 VKAPI_CALL callback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
      VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData,
      void *pUserData) {
  std::cout << "------------------------------------------------" << std::endl;
  std::cout << "Severity: " << string_VkDebugUtilsMessageSeverityFlagBitsEXT(messageSeverity) << std::endl;
  std::cout << "Message: " << pCallbackData->pMessage << std::endl;
  std::cout << "Type: " << string_VkDebugUtilsMessageTypeFlagsEXT(messageType) << std::endl;
  std::cout << "UserData: " << &pUserData << std::endl;
  std::cout << "------------------------------------------------" << std::endl;
  return VK_FALSE;
}


VkResult create(VkInstance instance, 
                VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                const VkAllocationCallbacks *pAllocator,
                VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr)
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  else
    return VK_ERROR_EXTENSION_NOT_PRESENT;
}

void populate(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
  createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
  createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                               VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
  createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                           VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
  createInfo.pfnUserCallback = DebugMessenger::callback;
  createInfo.pUserData = nullptr;
  createInfo.pNext = NULL;
}

void destroy(VkInstance instance,VkDebugUtilsMessengerEXT debugMessenger,
            const VkAllocationCallbacks *pAllocator) {
  auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(
      instance, "vkDestroyDebugUtilsMessengerEXT");

  if (func != nullptr)
    func(instance, debugMessenger, pAllocator);
}

} // namespace DebugMessenger

namespace Magma {

Device::Device(Window &window) {
  createInstance();
  setupDebugMessenger();

  window.createSurface(instance, &surface_);

  pickPhysicalDevice();
  vkGetPhysicalDeviceProperties(physicalDevice, &properties);
  createLogicalDevice();

  createCommandPool();
  createFence();

  instance_ = this;
}

Device::~Device() {
  vkDestroyCommandPool(device_, commandPool, nullptr);
  vkDestroyFence(device_, fence, nullptr);
  vkDestroyDevice(device_, nullptr);

  if (enableValidationLayers)
    DebugMessenger::destroy(instance, debugMessenger, nullptr);

  vkDestroySurfaceKHR(instance, surface_, nullptr);
  vkDestroyInstance(instance, nullptr);
  instance_ = nullptr;
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void Device::populateImGuiInitInfo(ImGui_ImplVulkan_InitInfo *init_info) {
  init_info->Instance = instance;
  init_info->PhysicalDevice = physicalDevice;
  init_info->Device = device_;
  init_info->QueueFamily =
      findQueueFamilies(physicalDevice).graphicsFamily.value();
  init_info->Queue = graphicsQueue_;
}

void Device::createImageWithInfo(const VkImageCreateInfo &imageInfo,
                                 VkMemoryPropertyFlags properties,
                                 VkImage &image, VkDeviceMemory &imageMemory) {
  if (vkCreateImage(device_, &imageInfo, nullptr, &image) != VK_SUCCESS)
    throw std::runtime_error("Failed to create Image!");

  VkMemoryRequirements memRequirements;
  vkGetImageMemoryRequirements(device_, image, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &imageMemory) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to allocate image memory!");

  if (vkBindImageMemory(device_, image, imageMemory, 0) != VK_SUCCESS)
    throw std::runtime_error("Failed to bind image memory!");
}

VkFormat Device::findSupportedFormat(const std::vector<VkFormat> &candidates,
                                     VkImageTiling tiling,
                                     VkFormatFeatureFlags features) {
  for (VkFormat format : candidates) {
    VkFormatProperties props;
    vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);

    if (tiling == VK_IMAGE_TILING_LINEAR &&
        (props.linearTilingFeatures & features) == features)
      return format;
    else if (tiling == VK_IMAGE_TILING_OPTIMAL &&
             (props.optimalTilingFeatures & features) == features)
      return format;
  }

  throw std::runtime_error("Failed to find a supported format!");
}

// Buffer
void Device::createBuffer(VkDeviceSize size, VkBufferUsageFlags usage,
                          VkMemoryPropertyFlags properties, VkBuffer &buffer,
                          VkDeviceMemory &bufferMemory) {
  VkBufferCreateInfo bufferInfo = {};
  bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
  bufferInfo.size = size;
  bufferInfo.usage = usage;
  bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

  if (vkCreateBuffer(device_, &bufferInfo, nullptr, &buffer) != VK_SUCCESS)
    throw std::runtime_error("Failed to create buffer!");

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(device_, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
  allocInfo.allocationSize = memRequirements.size;
  allocInfo.memoryTypeIndex =
      findMemoryType(memRequirements.memoryTypeBits, properties);

  if (vkAllocateMemory(device_, &allocInfo, nullptr, &bufferMemory) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to allocate buffer memory!");

  vkBindBufferMemory(device_, buffer, bufferMemory, 0);
}

void Device::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer,
                        VkDeviceSize size, VkDeviceSize srcOffset,
                        VkDeviceSize dstOffset) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferCopy copyRegion = {};
  copyRegion.srcOffset = srcOffset;
  copyRegion.dstOffset = dstOffset;
  copyRegion.size = size;
  vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

  endSingleTimeCommands(commandBuffer);
}

void Device::copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width,
                               uint32_t height) {
  VkCommandBuffer commandBuffer = beginSingleTimeCommands();

  VkBufferImageCopy region = {};
  region.bufferOffset = 0;
  region.bufferRowLength = 0;
  region.bufferImageHeight = 0;

  region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
  region.imageSubresource.mipLevel = 0;
  region.imageSubresource.baseArrayLayer = 0;
  region.imageSubresource.layerCount = 1;

  region.imageOffset = {0, 0, 0};
  region.imageExtent = {width, height, 1};

  vkCmdCopyBufferToImage(commandBuffer, buffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

  endSingleTimeCommands(commandBuffer);
}

void Device::copyImageToBuffer(VkCommandBuffer &commandBuffer,
                               VkBuffer dstBuffer, VkImage image,
                               VkBufferImageCopy region) {
  vkCmdCopyImageToBuffer(commandBuffer, image,
                         VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, dstBuffer, 1,
                         &region);
}

// Image Layout Transition
void Device::transitionImageLayout(VkImage image, VkImageLayout oldLayout,
                                   VkImageLayout newLayout,
                                   VkPipelineStageFlags srcStage,
                                   VkPipelineStageFlags dstStage,
                                   VkAccessFlags srcAccessMask,
                                   VkAccessFlags dstAccessMask,
                                   VkImageAspectFlags aspectMask) {
  VkImageMemoryBarrier barrier{VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER};
  barrier.oldLayout = oldLayout;
  barrier.newLayout = newLayout;
  barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
  barrier.image = image;
  barrier.subresourceRange.aspectMask = aspectMask;
  barrier.subresourceRange.baseMipLevel = 0;
  barrier.subresourceRange.levelCount = 1;
  barrier.subresourceRange.baseArrayLayer = 0;
  barrier.subresourceRange.layerCount = 1;
  barrier.srcAccessMask = srcAccessMask;
  barrier.dstAccessMask = dstAccessMask;

  vkCmdPipelineBarrier(FrameInfo::commandBuffer, srcStage, dstStage, 0, 0,
                       nullptr, 0, nullptr, 1, &barrier);
}

// Command Buffers
VkCommandBuffer Device::allocateCommandBuffer(VkCommandBufferLevel level) {
  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandBufferCount = 1;
  allocInfo.commandPool = commandPool;
  allocInfo.level = level;

  VkCommandBuffer commandBuffer;
  vkAllocateCommandBuffers(device_, &allocInfo, &commandBuffer);

  return commandBuffer;
}

VkCommandBuffer Device::beginSingleTimeCommands() {
  VkCommandBuffer commandBuffer =
      allocateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
  beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

  vkBeginCommandBuffer(commandBuffer, &beginInfo);
  return commandBuffer;
}

void Device::submitCommands(VkCommandBuffer &commandBuffer) {
  vkEndCommandBuffer(commandBuffer);

  VkSubmitInfo submitInfo{};
  submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
  submitInfo.commandBufferCount = 1;
  submitInfo.pCommandBuffers = &commandBuffer;

  if (vkQueueSubmit(graphicsQueue(), 1, &submitInfo, fence) != VK_SUCCESS)
    throw std::runtime_error("Failed to submit command buffer!");

  vkWaitForFences(device_, 1, &fence, VK_TRUE, UINT64_MAX);
  vkResetFences(device_, 1, &fence);
}

void Device::endSingleTimeCommands(VkCommandBuffer &commandBuffer) {
  submitCommands(commandBuffer);

  vkFreeCommandBuffers(device_, commandPool, 1, &commandBuffer);
}

// Depth Format
VkFormat Device::findDepthFormat() {
  return findSupportedFormat(
      {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT,
       VK_FORMAT_D24_UNORM_S8_UINT},
      VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

// --------------------------------------------------------------------------
// Private Methods
// --------------------------------------------------------------------------

bool Device::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  std::vector<VkLayerProperties> availableLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

  for (const char *layerName : validationLayers) {
    bool layerFound = false;

    for (const auto &layerProperty : availableLayers) {
      if (strcmp(layerName, layerProperty.layerName) == 0) {
        layerFound = true;
        break;
      }
    }

    if (!layerFound)
      return false;
  }
  return true;
}

void Device::createInstance() {
  if (enableValidationLayers && !checkValidationLayerSupport())
    throw std::runtime_error("Requested validation layers are not supported!");

  VkApplicationInfo appInfo = {};
  appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
  appInfo.pApplicationName = "Vulkan Application";
  appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.pEngineName = "No Engine";
  appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
  appInfo.apiVersion = VK_API_VERSION_1_3;
  appInfo.pNext = nullptr;

  std::vector<const char *> instanceExtensions = getRequiredExtensions();
  std::println("Requested Extensions:");
  for (const auto &extension : instanceExtensions) 
    std::println("\t{}", std::string(extension));

  VkInstanceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
  createInfo.pApplicationInfo = &appInfo;
  createInfo.enabledExtensionCount = static_cast<uint32_t>(instanceExtensions.size());
  createInfo.ppEnabledExtensionNames = instanceExtensions.data();

  VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
  if (enableValidationLayers) {
    createInfo.enabledLayerCount =
        static_cast<uint32_t>(validationLayers.size());
    createInfo.ppEnabledLayerNames = validationLayers.data();

    DebugMessenger::populate(debugCreateInfo);
    createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
  } else {
    createInfo.enabledLayerCount = 0;
    createInfo.pNext = nullptr;
  }

  if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS)
    throw std::runtime_error("Failed to create Vulkan instance!");
}

std::vector<const char *> Device::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
  std::vector<const char *> extensions(glfwExtensions,
                                  glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers)
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

  return extensions;
}

void Device::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  DebugMessenger::populate(createInfo);

  if (DebugMessenger::create(instance, &createInfo, nullptr,
                                   &debugMessenger) != VK_SUCCESS)
    throw std::runtime_error("Failed to set up debug messenger!");
}

// Physical Device
void Device::pickPhysicalDevice() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0)
    throw std::runtime_error("Failed to find a GPU that supports Vulkan!");

  std::vector<VkPhysicalDevice> devices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

  for (const auto &device : devices) {
    if (isDeviceSuitable(device)) {
      physicalDevice = device;
      break;
    }
  }

  if (physicalDevice == VK_NULL_HANDLE)
    throw std::runtime_error("No device was suitable!");
}

bool Device::isDeviceSuitable(VkPhysicalDevice device) {
  QueueFamilyIndices indices = findQueueFamilies(device);
  if (!indices.isComplete())
    return false;

  if (!checkDeviceExtensionSupport(device))
    return false;

  SwapchainSupportDetails swapChainSupport = querySwapChainSupport(device);
  bool swapChainAdequat = !swapChainSupport.formats.empty() &&
                     !swapChainSupport.presentModes.empty();
  if (!swapChainAdequat)
    return false;

  VkPhysicalDeviceFeatures supportedFeatures;
  vkGetPhysicalDeviceFeatures(device, &supportedFeatures);
  if(!supportedFeatures.samplerAnisotropy)
    return false;

  return true;
}

SwapchainSupportDetails Device::querySwapChainSupport(VkPhysicalDevice device) {
  SwapchainSupportDetails details;
  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface_,
                                            &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface_, &formatCount,
                                         details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface_, &presentModeCount,
                                            nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(
        device, surface_, &presentModeCount, details.presentModes.data());
  }

  return details;
}

bool Device::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount,
                                       availableExtensions.data());

  std::set<std::string> requiredExtensions(deviceExtensions.begin(),
                                      deviceExtensions.end());

  for (const auto &extension : availableExtensions) 
    requiredExtensions.erase(extension.extensionName);

  return requiredExtensions.empty();
}

uint32_t Device::findMemoryType(uint32_t typeFilter,
                                VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) &&
        (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
      return i;
  }

  throw std::runtime_error("Failed to find suitable memory type!");
}

// Logical Device
void Device::createLogicalDevice() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
  std::set<uint32_t> uniqueQueueFamilies = {indices.presentFamily.value(),
                                       indices.graphicsFamily.value()};

  float queuePriority = 1.0f;
  for (uint32_t queueFamily : uniqueQueueFamilies) {
    VkDeviceQueueCreateInfo queueCreateInfo = {};
    queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo.queueFamilyIndex = queueFamily;
    queueCreateInfo.queueCount = 1;
    queueCreateInfo.pQueuePriorities = &queuePriority;

    queueCreateInfos.push_back(queueCreateInfo);
  }

  VkPhysicalDeviceFeatures deviceFeatures = {};
  deviceFeatures.samplerAnisotropy = VK_TRUE;

  VkPhysicalDeviceVulkan13Features vulkan13Features = {};
  vulkan13Features.sType =
      VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES;
  vulkan13Features.dynamicRendering = VK_TRUE;
  vulkan13Features.synchronization2 = VK_TRUE;

  VkDeviceCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
  createInfo.queueCreateInfoCount =
      static_cast<uint32_t>(queueCreateInfos.size());
  createInfo.pQueueCreateInfos = queueCreateInfos.data();
  createInfo.enabledExtensionCount =
      static_cast<uint32_t>(deviceExtensions.size());
  createInfo.ppEnabledExtensionNames = deviceExtensions.data();
  createInfo.pEnabledFeatures = &deviceFeatures;
  createInfo.pNext = &vulkan13Features;

  if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device_) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create logical device!");

  vkGetDeviceQueue(device_, indices.graphicsFamily.value(), 0, &graphicsQueue_);
  vkGetDeviceQueue(device_, indices.presentFamily.value(), 0, &presentQueue_);
}

// Command Pool
void Device::createCommandPool() {
  QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

  VkCommandPoolCreateInfo poolInfo = {};
  poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
  poolInfo.queueFamilyIndex = indices.graphicsFamily.value();
  poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT |
                   VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;

  if (vkCreateCommandPool(device_, &poolInfo, nullptr, &commandPool) !=
      VK_SUCCESS)
    throw std::runtime_error("Failed to create command pool!");
}

// Fence
void Device::createFence() {
  VkFenceCreateInfo fenceInfo{};
  fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;

  if (vkCreateFence(device_, &fenceInfo, nullptr, &fence) != VK_SUCCESS)
    throw std::runtime_error("Failed to create fence!");
}

// Queue Families
QueueFamilyIndices Device::findQueueFamilies(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount,
                                           queueFamilies.data());

  int i = 0;
  for (const auto &queueFamily : queueFamilies) {
    if (queueFamily.queueCount > 0 &&
        queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT)
      indices.graphicsFamily = i;

    VkBool32 presentSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface_, &presentSupport);

    if (queueFamily.queueCount > 0 && presentSupport)
      indices.presentFamily = i;

    if (indices.isComplete())
      break;

    i++;
  }

  return indices;
}


} // namespace Magma
