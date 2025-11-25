#include "src/engine/engine.hpp"
#include "src/engine/specifications.hpp"
#include <print>

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
    std::println("Error: {}", e.what());
    fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
  }
}
