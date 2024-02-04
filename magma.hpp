#pragma once

#include "magma_window.hpp"

namespace magma {
class Magma {
public:
  static constexpr int WIDTH = 800;
  static constexpr int HEIGHT = 600;

  void run();

private:
  MagmaWindow magmaWindow{WIDTH, HEIGHT, "MAGMA!"};
};

} // namespace magma
