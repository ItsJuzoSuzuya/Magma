#include "window.hpp"
#include "../engine/specifications.hpp"
#include <GLFW/glfw3.h>
#include <cstring>
#include <stdexcept>

using namespace std;
namespace Magma {

Window::Window(EngineSpecifications &spec)
    : width(spec.windowWidth), height(spec.windowHeight), name(spec.name) {
  initGLFWWindow();
}

Window::~Window() {
  glfwDestroyWindow(window);
  glfwTerminate();
}
// ----------------------------------------------------------------------------
// Public Methods 
// ----------------------------------------------------------------------------

void Window::createSurface(VkInstance instance, VkSurfaceKHR *surface) {
  if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
    throw runtime_error("Failed to create window surface!");
}

void Window::close() { glfwSetWindowShouldClose(window, GLFW_TRUE); }

// ----------------------------------------------------------------------------
// Private Methods
// ----------------------------------------------------------------------------

void Window::initGLFWWindow() {
  glfwInit();

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window = glfwCreateWindow(width, height, name.c_str(), nullptr, nullptr);
  glfwSetWindowUserPointer(window, this);
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

  if (glfwRawMouseMotionSupported())
    glfwSetInputMode(window, GLFW_RAW_MOUSE_MOTION, GLFW_TRUE);

  glfwSetDropCallback(window, dropCallback);
}

// Static Callbacks
void Window::framebufferResizeCallback(GLFWwindow *window, int width,
                                       int height) {
  auto app = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
  app->framebufferResized = true;
  app->width = width;
  app->height = height;
}

void Window::dropCallback(GLFWwindow *window, int count, const char **paths) {
  if (count > 0) {
    const char *needle = "Magma/";
    const char *last = strstr(paths[0], needle);
    if (last == nullptr)
      return;

    last += strlen(needle);

    droppedText = string(last);
    hasDroppedText = true;
  }
}

} // namespace Magma
