#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_vulkan.h"
#include "src/engine/engine.hpp"
#include "src/engine/specifications.hpp"
#include <stdlib.h> // abort
#define GLFW_INCLUDE_NONE
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

// Volk headers
#ifdef IMGUI_IMPL_VULKAN_USE_VOLK
#define VOLK_IMPLEMENTATION
#include <volk.h>
#endif

#ifdef _DEBUG
#define APP_USE_VULKAN_DEBUG_REPORT
static VkDebugReportCallbackEXT g_DebugReport = VK_NULL_HANDLE;
#endif

// Main code
int main(int, char **) {
  Magma::EngineSpecifications spec{};
  spec.name = "Magma";
  spec.windowWidth = 1920;
  spec.windowHeight = 1080;

  try {
    Magma::Engine engine{spec};
    engine.run();
  } catch (const std::exception &e) {
    fprintf(stderr, "Error: %s\n", e.what());
    return EXIT_FAILURE;
  }
}
