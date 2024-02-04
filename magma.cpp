#include "magma.hpp"
#include <GLFW/glfw3.h>

namespace magma {

void Magma::run() {
  while (!magmaWindow.shouldClose()) {
    glfwPollEvents();
  }
}
} // namespace magma
