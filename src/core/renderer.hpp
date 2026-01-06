#pragma once
#include "core/swapchain.hpp"
#include "engine/render/pipeline_shader_info.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class IRenderer {
public:
  virtual ~IRenderer() = default;
  virtual void destroy() = 0;

  virtual VkPipelineLayout getPipelineLayout() const = 0;

  // Rendering
  virtual void onResize(const VkExtent2D extent) = 0;
  virtual void onRender() = 0;

  virtual bool isSwapChainDependent() const = 0;
  virtual SwapChain* getSwapChain() const = 0;

private:
  virtual void begin() = 0;
  virtual void record() = 0;
  virtual void end() = 0;

  virtual void createPipelineLayout(
                      const std::vector<VkDescriptorSetLayout> &layouts) = 0;
  virtual void createPipeline() = 0;
};
} // namespace Magma
