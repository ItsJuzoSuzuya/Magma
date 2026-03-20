module; 
#define GLFW_INCLUDE_VULKAN

#include "GLFW/glfw3.h"
#include "vulkan/vulkan.h"

module core:window;
import std;
import engine;

namespace Magma {

class EngineSpecifications;

export class Window {
public:
  Window(EngineSpecifications &spec): width(spec.windowWidth), height(spec.windowHeight), name(spec.name) {
    initGLFWWindow();
  }

  ~Window(){
    glfwDestroyWindow(window);
    glfwTerminate();
  }

  Window(const Window &) = delete;
  Window &operator=(const Window &) = delete;

  void createSurface(VkInstance instance, VkSurfaceKHR *surface){
    if (glfwCreateWindowSurface(instance, window, nullptr, surface) != VK_SUCCESS)
      throw runtime_error("Failed to create window surface!");
  }

  VkExtent2D getExtent() {
    return {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};
  }
  GLFWwindow *getGLFWwindow() const { return window; }
  static std::string getDroppedText() { return droppedText; }

  bool wasWindowResized() { return framebufferResized; }
  void resetWindowResizedFlag() { framebufferResized = false; }

  inline static bool hasDroppedText = false;
  static void resetHasDropped() { hasDroppedText = false; }

  bool shouldClose() { return glfwWindowShouldClose(window); }
  void close(){ glfwSetWindowShouldClose(window, GLFW_TRUE); }


private:
  int width;
  int height;
  bool framebufferResized = false;

  std::string name;
  GLFWwindow *window;
  void initGLFWWindow(){
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

  inline static std::string droppedText = "";
  static void framebufferResizeCallback(GLFWwindow *window, int width,
                                        int height){
    auto app = reinterpret_cast<Window *>(glfwGetWindowUserPointer(window));
    app->framebufferResized = true;
    app->width = width;
    app->height = height;
  }
  static void dropCallback(GLFWwindow *window, int count, const char **paths){
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

};
} // namespace Magma
