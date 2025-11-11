#pragma once
#include "descriptors.hpp"
#include "pipeline.hpp"
#include "render_target.hpp"
#include "swapchain.hpp"
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Window;
class Buffer;

struct RendererType {
  enum Onscreen { SwapChain };
  enum OffScreen {};
};

class Renderer {
public:
  // Constructor and destructor
  Renderer() = default;
  void init(VkDescriptorSetLayout descriptorSetLayout);
  virtual ~Renderer();

  // Delete copy constructor and assignment operator
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  // Getters
  VkPipelineLayout getPipelineLayout() { return pipelineLayout; }
  virtual Buffer *getCameraBuffer() { return nullptr; }

  // Rendering
  virtual void begin() = 0;
  virtual void record() = 0;
  virtual void end() = 0;

  // Final rendering
  void endFrame();

protected:
  // Pipeline
  std::unique_ptr<Pipeline> pipeline;
  void createPipeline(RenderTarget *renderTarget, const std::string &vertFile = "src/shaders/shader.vert.spv",
                      const std::string &fragFile = "src/shaders/shader.frag.spv");

  // Descriptor Pool
  std::unique_ptr<DescriptorPool> descriptorPool;
  std::unique_ptr<DescriptorSetLayout> descriptorSetLayout;
  virtual void createDescriptorPool() = 0;
  virtual void createDescriptorSetLayout() = 0;

private:
  // Pipeline Layout
  VkPipelineLayout pipelineLayout;
  void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
};
} // namespace Magma
