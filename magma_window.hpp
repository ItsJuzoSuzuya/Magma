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

  void createWindowSurface(VkInstance instance, VkSurfaceKHR *surface);

private:
  void initWindow();
  const int width;
  const int height;

  std::string windowName;
  GLFWwindow *window;
};

} // namespace magma
