#include <cstdint>
#include <string>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace magma {

class MagmaWindow {
public:
  MagmaWindow(int w, int h, std::string name);
  ~MagmaWindow();

  // Deletes copy constructors
  MagmaWindow(const MagmaWindow &) = delete;
  MagmaWindow &operator=(const MagmaWindow &) = delete;

  bool shouldClose() { return glfwWindowShouldClose(window); }
  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }
  bool wasWindowResized() { return frameBufferResized; }
  void resetWindowResizedFlag() { frameBufferResized = false; }
  GLFWwindow *getGLFWwindow() const { return window; }

  void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

private:
  static void frameBufferResizeCallback(GLFWwindow *window, int width,
                                        int height);
  void initWindow();

  int width;
  int height;
  bool frameBufferResized = false;

  std::string windowName;
  GLFWwindow *window;
};

} // namespace magma
