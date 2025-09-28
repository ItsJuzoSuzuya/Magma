#pragma once
#include "buffer.hpp"
#include "descriptors.hpp"
#include "device.hpp"
#include "imgui_impl_vulkan.h"
#include "renderer.hpp"
#include "swapchain.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>
namespace Magma {

class Window;

class RenderPipeline {
public:
  RenderPipeline(Window &window);
  ~RenderPipeline();

  // Getters
  ImGui_ImplVulkan_InitInfo getImGuiInitInfo();
  Device &getDevice() { return *device; }

  // Render
  void renderFrame();

private:
  // Device
  std::unique_ptr<Device> device = nullptr;

  // UBOs
  std::vector<std::unique_ptr<Buffer>> uboBuffers;

  // Descriptors
  std::unique_ptr<DescriptorPool> descriptorPool = nullptr;
  std::unique_ptr<DescriptorSetLayout> globalSetLayout = nullptr;
  std::vector<VkDescriptorSet> globalDescriptorSets;
  void createDescriptorPool();

  // Renderer
  std::unique_ptr<Renderer> renderer = nullptr;
  bool firstFrame = true;

  // ImGui Dockspace
  void createDockspace(ImGuiID &dockspace_id);
};
} // namespace Magma
