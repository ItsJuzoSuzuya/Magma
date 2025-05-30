#include "movement_controller.hpp"
#include "components/transform.hpp"
#include "core/game_object.hpp"
#include <GLFW/glfw3.h>
#include <iostream>

using namespace std;
namespace magma {

void MovementController::move(GLFWwindow *window, GameObject &object,
                              float dt) {
  if (firstMouse) {
    glfwGetCursorPos(window, &mouseX, &mouseY);
    firstMouse = false;
    return;
  }

  handleInput(window, object, dt);
};

void MovementController::handleInput(GLFWwindow *window, GameObject &object,
                                     float dt) {
  double newMouseX, newMouseY;
  glfwGetCursorPos(window, &newMouseX, &newMouseY);

  double deltaX = newMouseX - mouseX;
  double deltaY = newMouseY - mouseY;

  glm::vec3 rotateDirection{0.f};
  rotateDirection.x = deltaY * dt;
  rotateDirection.y = deltaX * dt;

  Transform &transform = *object.getComponent<Transform>();

  glm::vec3 direction = {
      glm::cos(transform.rotation.x) * glm::sin(transform.rotation.y),
      glm::sin(transform.rotation.x),
      glm::cos(transform.rotation.x) * glm::cos(transform.rotation.y)};

  float yaw = transform.rotation.y;

  const glm::vec3 forwardDirection{sin(yaw), 0, cos(yaw)};
  const glm::vec3 rightDirection{forwardDirection.z, 0, -forwardDirection.x};

  glm::vec3 moveDirection{0.f};
  if (glfwGetKey(window, keys.left) == GLFW_PRESS)
    moveDirection -= rightDirection;
  if (glfwGetKey(window, keys.right) == GLFW_PRESS)
    moveDirection += rightDirection;
  if (glfwGetKey(window, keys.forward) == GLFW_PRESS)
    moveDirection += forwardDirection;
  if (glfwGetKey(window, keys.backward) == GLFW_PRESS)
    moveDirection -= forwardDirection;

  transform.position.x += moveDirection.x / 100.f;
  transform.position.z += moveDirection.z / 100.f;

  mouseX = newMouseX;
  mouseY = newMouseY;
}
} // namespace magma
