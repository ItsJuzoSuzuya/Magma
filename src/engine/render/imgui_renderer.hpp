#pragma once
#include "../../core/renderer.hpp"
#include "../widgets/widget.hpp"

namespace Magma {

class ImGuiRenderer : public Renderer {
public:
  ImGuiRenderer(Device &device, SwapChain &swapChain,
                VkDescriptorSetLayout descriptorSetLayout);

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
  std::vector<std::unique_ptr<Widget>> widgets;
  bool dockBuilt = false;
};

} // namespace Magma
