#ifndef WINDOW_HPP
#define WINDOW_HPP
#include <iostream>
#include <stdexcept>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace magma {

class Window {
public:
  Window(int width, int height, std::string name);
  ~Window();

  bool shouldClose() { return glfwWindowShouldClose(window); }

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }
  GLFWwindow *getGLFWwindow() { return window; }

  void createSurface(VkInstance instance, VkSurfaceKHR *surface) {
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) !=
        VK_SUCCESS)
      throw std::runtime_error("Surface Creation was unsuccessful");
  }

  bool wasWindowResized() { return framebufferResized; }
  void resetWindowResizedFlag() { framebufferResized = false; }

  void close();

private:
  void initWindow();

  int width;
  int height;
  bool framebufferResized = false;

  std::string name;
  GLFWwindow *window;

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
};
} // namespace magma
#endif
