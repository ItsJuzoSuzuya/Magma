#pragma once

#include <vulkan/vulkan.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <string>

namespace Magma {

class EngineSpecifications;

class Window {
public:
  /** Constructs the window with the given specifications */
  Window(EngineSpecifications &spec);
  ~Window();

  // Delete copy constructors
  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  /** Creates the Vulkan surface for this window */
  void createSurface(VkInstance instance, VkSurfaceKHR *surface);

  // Getters
  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }
  GLFWwindow *getGLFWwindow() { return window; }
  static std::string getDroppedText() { return droppedText; }

  /** Checks if the window was resized since the last check */
  bool wasWindowResized() { return framebufferResized; }
  /** Resets the window resized flag */
  void resetWindowResizedFlag() { framebufferResized = false; }
  /** Resets the drop text flag */
  static void resetDrop() { hasDroppedText = false; }

  /** Checks if the window should close */
  bool shouldClose() { return glfwWindowShouldClose(window); }
  /** Closes the window */
  void close();

  inline static bool hasDroppedText = false;

private:
  // --- Window properties ---
  int width;
  int height;
  bool framebufferResized = false;

  // --- GLFW window ---
  std::string name;
  GLFWwindow *window;

  /** Initializes the GLFW window */
  void initWindow();

  //  --- Callbacks ---
  /** Callback for framebuffer resize events */
  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
  /** Callback for file drop events */
  static void dropCallback(GLFWwindow *window, int count, const char **paths);
  inline static std::string droppedText = "";
};
} // namespace Magma
