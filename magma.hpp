#pragma once

#include "magma_device.hpp"
#include "magma_model.hpp"
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
  void loadModels();
  void calculateSiepinskiTriangle(std::vector<MagmaModel::Vertex> preVertices,
                                  std::vector<MagmaModel::Vertex> *result,
                                  int counter);
  void createPipelineLayout();
  void createPipeline();
  void createCommandBuffers();
  void freeCommandBuffers();
  void drawFrame();
  void recreateSwapChain();
  void recordCommandBuffer(int imageIndex);

  MagmaWindow magmaWindow{WIDTH, HEIGHT, "MAGMA!"};
  MagmaDevice magmaDevice{magmaWindow};
  std::unique_ptr<MagmaSwapChain> magmaSwapChain;
  std::unique_ptr<MagmaPipeline> magmaPipeline;
  VkPipelineLayout pipelineLayout;
  std::vector<VkCommandBuffer> commandBuffers;
  std::unique_ptr<MagmaModel> magmaModel;
};

} // namespace magma
