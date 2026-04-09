module;
#include <glm/ext/matrix_float4x4.hpp>

export module engine:editor_camera;
import core;
import components;
import :gameobject;

namespace Magma {

/**
 * EditorCamera wraps a GameObject that has Camera and Transform components,
 * providing movement APIs for use in the editor viewport.
 */
export class EditorCamera {
public:
  EditorCamera() = default;
  EditorCamera(GameObject *obj) : cameraObject(obj) {
    if (obj) {
      transform = obj->getComponent<Transform>();
      camera = obj->getComponent<Camera>();
    }
  }

  ~EditorCamera() = default;

  void onUpdate() {
    if (camera)
      camera->onUpdate();
  }

  RenderProxy collectProxy() const {
    RenderProxy proxy = {};
    if (camera)
      camera->collectProxy(proxy);
    return proxy;
  }

  void setPerspectiveProjection(float fov, float aspect, float near, float far) {
    if (camera)
      camera->setPerspectiveProjection(fov, aspect, near, far);
  }

  void setAspectRatio(float aspect) {
    if (camera)
      camera->setAspectRatio(aspect);
  }

  void moveRight(float speed) {
    if (cameraObject)
      cameraObject->moveRight(speed);
  }

  void moveForward(float speed) {
    if (cameraObject)
      cameraObject->moveForward(speed);
  }

  void moveUp(float speed) {
    if (cameraObject)
      cameraObject->moveUp(speed);
  }

  Transform *getTransform() { return transform; }
  Camera *getCamera() { return camera; }
  const glm::mat4 &getProjection() const {
    static glm::mat4 identity{1.f};
    return camera ? camera->getProjection() : identity;
  }
  const glm::mat4 &getView() const {
    static glm::mat4 identity{1.f};
    return camera ? camera->getView() : identity;
  }

private:
  GameObject *cameraObject = nullptr;
  Transform *transform = nullptr;
  Camera *camera = nullptr;
};

} // namespace Magma
