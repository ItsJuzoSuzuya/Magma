#include "editor_camera.hpp"
#include "engine/scene.hpp"
#include "gameobject.hpp"

using namespace std;
namespace Magma {

EditorCamera::EditorCamera(): GameObject(UINT32_MAX, "Editor Camera") {
  // Create a transform and a camera bound to it
  transform = addComponent<Transform>();
  camera = addComponent<Camera>();
  camera->setPerspectiveProjection(glm::radians(90.f), 16.f / 9.f, 0.1f, 100.f);
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

void EditorCamera::setPerspectiveProjection(float fov, float aspect, float near,
                                            float far) {
  camera->setPerspectiveProjection(fov, aspect, near, far);
}

void EditorCamera::setAspectRatio(float aspect) {
  camera->setAspectRatio(aspect);
}

} // namespace Magma
