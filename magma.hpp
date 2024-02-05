#pragma once

#include "magma_device.hpp"
#include "magma_pipeline.hpp"

namespace magma {
class Magma {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

  void run();

private:
  MagmaWindow magmaWindow{WIDTH, HEIGHT, "MAGMA!"};
  MagmaDevice magmaDevice{magmaWindow};
  MagmaPipeline magmaPipeline{
      magmaDevice, "shaders/simple_shader.vert.spv",
      "shaders/simple_shader.frag.spv",
      MagmaPipeline::defaultPipelineConfigInfo(WIDTH, HEIGHT)};
};

} // namespace magma
