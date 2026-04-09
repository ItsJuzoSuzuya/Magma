#include <cstdlib>
#include <exception>
#include <print>

import core;
import engine;
import ui_manager;
import widgets;

int main(int, char **) {
  Magma::WindowSpecification spec{};
  spec.name = "Magma";
  spec.windowWidth = 1280;
  spec.windowHeight = 700;

  Magma::Window window{spec};

  try {
    Magma::Engine engine{&window};

    #if defined(MAGMA_WITH_EDITOR)
      Magma::WidgetManager widgetManager{};
      Magma::UI_Manager uiManager{};
      std::println("UI_Manager setup!");

      auto imguiRenderer = uiManager.setupUI(&window, &widgetManager);
      std::println("imguiRenderer setup!");
      imguiRenderer->onDraw = [&widgetManager]() {
        widgetManager.drawWidgets();
      };
      std::println("imguiRenderer onDraw!");

      engine.addRenderer(std::move(imguiRenderer));
      engine.addRenderer(uiManager.setupEditorRenderer());
      engine.addRenderer(uiManager.setupGameRenderer());

      engine.onEditorDock = [&widgetManager]() {
        widgetManager.dock();
      };
    #else
      Magma::UI_Manager uiManager{};
      engine.addRenderer(uiManager.setupEditorRenderer());
      engine.addRenderer(uiManager.setupGameRenderer());
    #endif

    engine.run();
  } catch (const std::exception &e) {
    std::println("Error: {}", e.what());
    fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
  }
}
