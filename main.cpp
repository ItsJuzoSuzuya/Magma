import engine:window_specification;
import engine:magma;

// Main code
int main(int, char **) {
  Magma::EngineSpecifications spec{};
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
