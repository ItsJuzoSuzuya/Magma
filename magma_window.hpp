#include <string>
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

private:
  void initWindow();
  const int width;
  const int height;

  std::string windowName;
  GLFWwindow *window;
};

} // namespace magma
