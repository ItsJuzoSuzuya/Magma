#include "components/component.hpp"
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

namespace magma {

class MovementController {
public:
  struct KeyMapping {
    int forward = GLFW_KEY_W;
    int backward = GLFW_KEY_S;
    int left = GLFW_KEY_A;
    int right = GLFW_KEY_D;
    int up = GLFW_KEY_SPACE;
    int down = GLFW_KEY_LEFT_SHIFT;
  };

  void move(GLFWwindow *window, GameObject &object, float dt);

private:
  KeyMapping keys{};
  double mouseX, mouseY;
  bool firstMouse = true;

  void handleInput(GLFWwindow *window, GameObject &object, float dt);
};

} // namespace magma
