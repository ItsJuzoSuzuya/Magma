#pragma once

#include "magma_device.hpp"
#include "magma_pipeline.hpp"
#include "magma_swap_chain.hpp"
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace magma {
class Magma {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

  Magma();
  ~Magma();

  Magma(const Magma &) = delete;
  Magma &operator=(const Magma &) = delete;

  void run();

private:
  void createPipelineLayout();
  void createPipeline();
  void createCommandBuffers();
  void drawFrame();

  MagmaWindow magmaWindow{WIDTH, HEIGHT, "MAGMA!"};
  MagmaDevice magmaDevice{magmaWindow};
  MagmaSwapChain magmaSwapChain{magmaDevice, magmaWindow.getExtent()};
  std::unique_ptr<MagmaPipeline> magmaPipeline;
  VkPipelineLayout pipelineLayout;
  std::vector<VkCommandBuffer> commandBuffers;
};

} // namespace magma
