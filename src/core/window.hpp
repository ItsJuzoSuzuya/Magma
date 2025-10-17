#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace Magma {

class EngineSpecifications;

class Window {
public:
  Window(EngineSpecifications &spec);
  ~Window();

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  void createSurface(VkInstance instance, VkSurfaceKHR *surface);

  // Getters
  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }
  GLFWwindow *getGLFWwindow() { return window; }
  static std::string getDroppedText() { return droppedText; }

  bool wasWindowResized() { return framebufferResized; }
  void resetWindowResizedFlag() { framebufferResized = false; }
  static void resetDrop() { hasDroppedText = false; }

  bool shouldClose() { return glfwWindowShouldClose(window); }
  void close();

  inline static bool hasDroppedText = false;

private:
  int width;
  int height;
  bool framebufferResized = false;

  std::string name;
  GLFWwindow *window;

  void initWindow();

  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
  static void dropCallback(GLFWwindow *window, int count, const char **paths);
  inline static std::string droppedText = "";
};
} // namespace Magma
