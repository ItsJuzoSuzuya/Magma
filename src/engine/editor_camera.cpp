#include "editor_camera.hpp"
#include <memory>
#include <stdexcept>

namespace Magma {

EditorCamera::EditorCamera(std::unique_ptr<GameObject> obj) : cameraObject(std::move(obj)) {
  if (cameraObject) {
    transform = cameraObject->getComponent<Transform>();
    camera = cameraObject->getComponent<Camera>();
    if (camera && transform) 
      camera->setOwnerTransform(transform);
    else throw std::runtime_error("EditorCamera constructed without Camera or Transform!");
  }
}

void EditorCamera::onUpdate() {
  if (camera) camera->onUpdate();
}

RenderProxy EditorCamera::collectProxy() const {
  RenderProxy proxy = {};
  if (camera) camera->collectProxy(proxy);
  return proxy;
}

void EditorCamera::setPerspectiveProjection(float fov, float aspect, float near, float far) {
  if (camera) camera->setPerspectiveProjection(fov, aspect, near, far);
}

void EditorCamera::setAspectRatio(float aspect) {
  if (camera) camera->setAspectRatio(aspect);
}

void EditorCamera::moveRight(float speed) {
  if (cameraObject) cameraObject->moveRight(speed);
}

void EditorCamera::moveForward(float speed) {
  if (cameraObject) cameraObject->moveForward(speed);
}

void EditorCamera::moveUp(float speed) {
  if (cameraObject) cameraObject->moveUp(speed);
}

const glm::mat4 &EditorCamera::getProjection() const {
  static glm::mat4 identity{1.f};
  return camera ? camera->getProjection() : identity;
}

const glm::mat4 &EditorCamera::getView() const {
  static glm::mat4 identity{1.f};
  return camera ? camera->getView() : identity;
}

}
