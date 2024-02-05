#include "magma_window.hpp"
#include <GLFW/glfw3.h>
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace magma {

MagmaWindow::MagmaWindow(int w, int h, std::string name)
    : width{w}, height{h}, windowName{name} {
  initWindow();
}

MagmaWindow::~MagmaWindow() {
  glfwDestroyWindow(window);
  glfwTerminate();
}

void MagmaWindow::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window =
      glfwCreateWindow(width, height, windowName.c_str(), nullptr, nullptr);
}

void MagmaWindow::createWindowSurface(VkInstance instance,
                                      VkSurfaceKHR *surface) {
  if (glfwCreateWindowSurface(instance, window, nullptr, surface) !=
      VK_SUCCESS) {
    throw std::runtime_error("failed to create window surface!");
  }
}

} // namespace magma
