#pragma once

#include "../magma_device.hpp"
#include "../magma_frame_info.hpp"
#include "../magma_game_object.hpp"
#include "../magma_pipeline.hpp"

// std
#include <memory>
#include <vector>

namespace magma {
class SimpleRenderSystem {
public:
  SimpleRenderSystem(MagmaDevice &device, VkRenderPass renderPass,
                     VkDescriptorSetLayout globalSetLayout);
  ~SimpleRenderSystem();

  SimpleRenderSystem(const SimpleRenderSystem &) = delete;
  SimpleRenderSystem &operator=(const SimpleRenderSystem &) = delete;

  void renderGameObjects(FrameInfo &frameInfo,
                         std::vector<MagmaGameObject> &gameObjects);

private:
  void createPipelineLayout(VkDescriptorSetLayout globalSetLayout);
  void createPipeline(VkRenderPass renderPass);

  MagmaDevice &magmaDevice;

  std::unique_ptr<MagmaPipeline> magmaPipeline;
  VkPipelineLayout pipelineLayout;
};
} // namespace magma
