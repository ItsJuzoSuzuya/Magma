module;
#include <glm/ext/matrix_float4x4.hpp>

export module engine:editor_camera;
import core;

namespace Magma {

/**
 * EditorCamera aggregates a Transform and a Camera for editor usage.
 * It owns the transform and camera instances and exposes simple movement
 * APIs for the editor (e.g., when user presses WASD in the offscreen view).
 */
export class EditorCamera {
public:
  EditorCamera(GameObject *obj) cameraObject(obj){}

  ~EditorCamera() = default;

  void onUpdate() {
    camera->onUpdate();
  }
  void onRender(IRenderer &renderer) {
    camera->onRender(renderer);
  }


  // Perspective config
  void setPerspectiveProjection(float fov, float aspect, float near,
                                              float far) {
    camera->setPerspectiveProjection(fov, aspect, near, far);
  }

  void setAspectRatio(float aspect) {
    camera->setAspectRatio(aspect);
  }

  // Accessors
  Transform *getTransform() { return transform; }
  Camera *getCamera() { return camera; }
  const glm::mat4 &getProjection() const { return camera->getProjection(); }
  const glm::mat4 &getView() const { 
    return camera->getView(); }

private:
  GameObject *cameraObject = nullptr;
  Transform *transform = nullptr;
  Camera *camera = nullptr;
};

} // namespace Magma
