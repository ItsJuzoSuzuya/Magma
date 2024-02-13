#pragma once

#include "magma_device.hpp"
#include "magma_game_object.hpp"
#include "magma_renderer.hpp"
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
  void loadGameObjects();
  void calculateSiepinskiTriangle(std::vector<MagmaModel::Vertex> preVertices,
                                  std::vector<MagmaModel::Vertex> *result,
                                  int counter);

  MagmaWindow magmaWindow{WIDTH, HEIGHT, "MAGMA!"};
  MagmaDevice magmaDevice{magmaWindow};
  MagmaRenderer magmaRenderer{magmaWindow, magmaDevice};

  std::vector<MagmaGameObject> gameObjects;
};

} // namespace magma
