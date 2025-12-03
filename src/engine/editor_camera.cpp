#include "editor_camera.hpp"
#include "../../core/renderer.hpp"
#include "gameobject.hpp"

using namespace std;
namespace Magma {

EditorCamera::EditorCamera() {
  // Create a transform and a camera bound to it
  cameraObject = new GameObject(UINT32_MAX, "Editor Camera");
  cameraObject->addComponent<Transform>();
  transform = cameraObject->getComponent<Transform>();
  cameraObject->addComponent<Camera>();
  camera = cameraObject->getComponent<Camera>();
  camera->setPerspectiveProjection(glm::radians(90.f), 16.f / 9.f, 0.1f, 100.f);
}

void EditorCamera::onUpdate() {
  // Keep camera's internal view in sync with transform
  camera->onUpdate();
}

void EditorCamera::onRender(Renderer &renderer) {
  // Push camera UBO to GPU before rendering scene
  camera->onRender(renderer);
}

void EditorCamera::moveRight(float speed) {
  transform->position += transform->right() * speed;
}

void EditorCamera::moveForward(float speed) {
  transform->position += transform->forward() * speed;
}

void EditorCamera::moveUp(float speed) {
  transform->position += transform->up() * speed;
}

void EditorCamera::setPerspectiveProjection(float fov, float aspect, float near, float far) {
  camera->setPerspectiveProjection(fov, aspect, near, far);
}

} // namespace Magma
