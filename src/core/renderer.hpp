#pragma once
#include "device.hpp"
#include "pipeline.hpp"
#include "render_target.hpp"
#include "swapchain.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Window;

struct RendererType {
  enum Onscreen { SwapChain };
  enum OffScreen {};
};

class Renderer {
public:
  // Constructor and destructor
  Renderer(Device &device, VkDescriptorSetLayout descriptorSetLayout);
  virtual ~Renderer();

  // Delete copy constructor and assignment operator
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  // Getters
  RenderTarget &target() { return *renderTarget; }
  VkRenderPass getRenderPass() { return renderTarget->getRenderPass(); }

  // Rendering
  virtual void begin() = 0;
  virtual void record() = 0;
  virtual void end() = 0;

  // Final rendering
  void endFrame();

protected:
  // Render Target
  std::unique_ptr<RenderTarget> renderTarget;

  // Pipeline
  std::unique_ptr<Pipeline> pipeline;
  void createPipeline();

private:
  Device &device;

  // Pipeline Layout
  VkPipelineLayout pipelineLayout;
  void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
};
} // namespace Magma
