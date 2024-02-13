
#pragma once

#include "magma_device.hpp"
#include "magma_game_object.hpp"
#include "magma_pipeline.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace magma {
class SimpleRenderSystem {
public:
  SimpleRenderSystem(MagmaDevice &device, VkRenderPass renderPass);
  ~SimpleRenderSystem();

  SimpleRenderSystem(const SimpleRenderSystem &) = delete;
  SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

  void renderGameObjects(VkCommandBuffer commandBuffer,
                         std::vector<MagmaGameObject> &gameObjects);

private:
  void createPipelineLayout();
  void createPipeline(VkRenderPass renderPass);

  MagmaDevice &magmaDevice;

  std::unique_ptr<MagmaPipeline> magmaPipeline;
  VkPipelineLayout pipelineLayout;
};

} // namespace magma
