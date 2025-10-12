#include "src/engine/engine.hpp"
#include "src/engine/specifications.hpp"
#include <stdlib.h>
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Main code
int main(int, char **) {
  Magma::EngineSpecifications spec{};
  spec.name = "Magma";
  spec.windowWidth = 1280;
  spec.windowHeight = 700;

  try {
    Magma::Engine engine{spec};
    engine.run();
  } catch (const std::exception &e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
  }
}
