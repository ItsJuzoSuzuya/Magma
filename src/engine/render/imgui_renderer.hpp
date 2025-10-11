#pragma once
#include "../../core/descriptors.hpp"
#include "../../core/renderer.hpp"
#include "../widgets/widget.hpp"
#include <memory>

namespace Magma {

class ImGuiRenderer : public Renderer {
public:
  ImGuiRenderer(Device &device, SwapChain &swapChain);

  // Getters
  VkDescriptorPool getDescriptorPool() const;

  // Widget management
  void addWidget(std::unique_ptr<Widget> widget);

  // Call at the beginning of each frame
  void newFrame();

  // Pre-frame: set up dockspace and let widgets run their pre-frame hooks
  // Returns false if any widget requested to skip the frame (e.g., resize)
  bool preFrame();

  // Rendering
  void begin() override;
  void record() override;
  void end() override;

  // Resize
  void resize(VkExtent2D extent, VkSwapchainKHR swapChain);

private:
  // Descriptor Pool
  std::unique_ptr<DescriptorPool> descriptorPool;
  std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
  void createDescriptorPool();
  void createDescriptorSetLayout();

  // Widgets
  std::vector<std::unique_ptr<Widget>> widgets;
  bool dockBuilt = false;
};

} // namespace Magma
