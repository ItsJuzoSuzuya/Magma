import engine:specification;
import engine:magma;
import core;

// Main code
int main(int, char **) {
  Magma::WindowSpecifications spec{};
  spec.name = "Magma";
  spec.windowWidth = 1280;
  spec.windowHeight = 700;

  Window window = {spec};

  try {
    Magma::Engine engine{&window};
    engine.run();
  } catch (const std::exception &e) {
    std::println("Error: {}", e.what());
    fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
  }
}
