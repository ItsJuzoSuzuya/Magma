#pragma once
#include "core/descriptors.hpp"
#include "core/pipeline.hpp"
#include "core/renderer.hpp"
#include "core/window.hpp"
#include "engine/widgets/widget.hpp"
#include "imgui_impl_vulkan.h"
#include "swapchain_target.hpp"
#include <memory>

namespace Magma {

class ImGuiRenderer : public IRenderer {
public:
  ImGuiRenderer(std::unique_ptr<SwapchainTarget> target, PipelineShaderInfo shaderInfo);
  void initImGui(const Window &window);

  ~ImGuiRenderer() override;
  void destroy() override;

  VkDescriptorPool getDescriptorPool() const;
  SwapchainTarget &target() { return *renderTarget; }
  VkPipelineLayout getPipelineLayout() const override {
    return pipelineLayout; }

  void addWidget(std::unique_ptr<Widget> widget);

  void newFrame();
  // Pre-frame: set up dockspace and let widgets run their pre-frame hooks
  // Returns false if any widget requested to skip the frame (e.g., resize)
  void preFrame();

  void onResize(VkExtent2D extent) override;
  void onRender() override;

  bool isSwapChainDependent() const override { return true; }
  SwapChain* getSwapChain() const override {
    return renderTarget->swapChain(); }

private:
  std::unique_ptr<SwapchainTarget> renderTarget;

  VkFormat imguiColorFormat;
  ImGui_ImplVulkan_InitInfo getImGuiInitInfo();

  void begin() override;
  void record() override;
  void end() override;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  void createPipelineLayout(
      const std::vector<VkDescriptorSetLayout> &layouts) override;

  std::unique_ptr<Pipeline> pipeline = nullptr;
  PipelineShaderInfo shaderInfo;
  void createPipeline() override;

  // Descriptor 
  std::unique_ptr<DescriptorPool> descriptorPool;
  std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
  void createDescriptorPool();
  void createDescriptorSetLayout();

  // Widgets
  std::vector<std::unique_ptr<Widget>> widgets;
  bool dockBuilt = false;
  void createDockspace(ImGuiID &dockspace_id, const ImVec2 &size);
};

} // namespace Magma
