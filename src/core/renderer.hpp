#pragma once
#include "../engine/components/camera.hpp"
#include "../engine/components/point_light.hpp"
#include "pipeline.hpp"
#include "render_target.hpp"
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
  Renderer() = default;
  void init(VkDescriptorSetLayout descriptorSetLayout);
  virtual ~Renderer();

  // Delete copy constructor and assignment operator
  Renderer(const Renderer &) = delete;
  Renderer &operator=(const Renderer &) = delete;

  // Getters
  VkPipelineLayout getPipelineLayout() { return pipelineLayout; }
  virtual Camera *getActiveCamera() { return nullptr; }

  // Rendering
  virtual void begin() = 0;
  virtual void record() = 0;
  virtual void end() = 0;

  // Final rendering
  void endFrame();

  virtual void uploadCameraUBO(const CameraUBO &ubo) {}
  virtual void submitPointLight(const PointLightData &lightData) {}

protected:
  // Pipeline
  std::unique_ptr<Pipeline> pipeline;
  void
  createPipeline(RenderTarget *renderTarget,
                 const std::string &vertFile = "src/shaders/shader.vert.spv",
                 const std::string &fragFile = "src/shaders/shader.frag.spv");

private:
  // Pipeline Layout
  VkPipelineLayout pipelineLayout;
  void createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout);
};
} // namespace Magma
