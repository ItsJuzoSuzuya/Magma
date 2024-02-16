#pragma once

#include "magma_descriptors.hpp"
#include "magma_device.hpp"
#include "magma_game_object.hpp"
#include "magma_renderer.hpp"

// std
#include <memory>
#include <vector>

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

  MagmaWindow magmaWindow{WIDTH, HEIGHT, "Magma!"};
  MagmaDevice magmaDevice{magmaWindow};
  MagmaRenderer magmaRenderer{magmaWindow, magmaDevice};

  // note: order of declarations matters
  std::unique_ptr<MagmaDescriptorPool> globalPool{};
  std::vector<MagmaGameObject> gameObjects;
};
} // namespace magma
