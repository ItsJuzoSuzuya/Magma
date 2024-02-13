#pragma once

#include "magma_game_object.hpp"
#include <GLFW/glfw3.h>
namespace magma {

class KeyboardMovementController {
public:
  struct KeyMappings {
    int moveLeft = GLFW_KEY_A;
    int moveRight = GLFW_KEY_D;
    int moveForward = GLFW_KEY_W;
    int moveBack = GLFW_KEY_S;
    int moveUp = GLFW_KEY_E;
    int moveDown = GLFW_KEY_Q;

    int lookLeft = GLFW_KEY_LEFT;
    int lookRight = GLFW_KEY_RIGHT;
    int lookUp = GLFW_KEY_UP;
    int lookDown = GLFW_KEY_DOWN;
  };

  void move(GLFWwindow *window, float dt, MagmaGameObject &gameobject);

  KeyMappings keys{};
  float moveSpeed{3.f};
  float lookSpeed{1.5f};
};
} // namespace magma
