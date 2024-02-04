#include "magma_window.hpp"
#include <GLFW/glfw3.h>

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
} // namespace magma
