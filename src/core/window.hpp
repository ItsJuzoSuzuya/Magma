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

  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }
  GLFWwindow *getGLFWwindow() { return window; }
  static std::string getDroppedText() { return droppedText; }

  bool wasWindowResized() { return framebufferResized; }
  void resetWindowResizedFlag() { framebufferResized = false; }

  inline static bool hasDroppedText = false;
  static void resetHasDropped() { hasDroppedText = false; }

  bool shouldClose() { return glfwWindowShouldClose(window); }
  void close();



private:
  int width;
  int height;
  bool framebufferResized = false;

  std::string name;
  GLFWwindow *window;
  void initGLFWWindow();

  inline static std::string droppedText = "";
  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
  static void dropCallback(GLFWwindow *window, int count, const char **paths);

};
} // namespace Magma
