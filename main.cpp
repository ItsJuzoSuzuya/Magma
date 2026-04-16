#include "core/window.hpp"
#include "core/specifications.hpp"
#include "engine/engine.hpp"
#include "engine/render/scene_renderer.hpp"
#include "engine/render/viewport.hpp"
#include "engine/ui.hpp"
#include "engine/widgets/game_editor.hpp"
#include "engine/widgets/game_view.hpp"
#include "engine/widgets/wiget_manger.hpp"
#include <print>
#include <utility>

// Main code
int main(int, char **) {
  Magma::WindowSpecification spec{};
  spec.name = "Magma";
  spec.windowWidth = 1280;
  spec.windowHeight = 700;

  try {
    Magma::Window window = {spec};
    Magma::Engine engine = {window};

    #if defined(MAGMA_WITH_EDITOR)
      Magma::SceneRenderer *gameRenderer = engine.createGameRenderer();
      Magma::SceneRenderer *editorRenderer = engine.createEditorRenderer();
      Magma::Viewport gameViewport = Magma::makeViewport(gameRenderer, false);
      Magma::Viewport editorViewport = Magma::makeViewport(editorRenderer, true);

      Magma::WidgetManager widgetManager{};
      widgetManager.addWidget(std::make_unique<Magma::GameView>(gameViewport));
      widgetManager.addWidget(std::make_unique<Magma::GameEditor>(editorViewport));

      auto imguiRenderer = Magma::UI::setup(&window, &widgetManager);
      engine.setImguiRenderer(std::move(imguiRenderer));
    #else
      SceneRenderer gameRenderer = engine.createGameRenderer();
      engine.addRenderer(std::move(gameRenderer));
    #endif

    engine.run();
  } catch (const std::exception &e) {
    std::println("Error: {}", e.what());
    fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
  }
}
