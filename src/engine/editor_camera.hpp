#pragma once
#include "components/camera.hpp"
#include "components/transform.hpp"

namespace Magma {

class SceneRenderer;
class GameObject;

/**
 * EditorCamera aggregates a Transform and a Camera for editor usage.
 * It owns the transform and camera instances and exposes simple movement
 * APIs for the editor (e.g., when user presses WASD in the offscreen view).
 */
class EditorCamera {
public:
  EditorCamera();
  ~EditorCamera() = default;

  // Lifecycle helpers
  void onUpdate();
  void onRender(SceneRenderer &renderer);

  // Movement helpers used by editor input
  void moveRight(float speed);
  void moveForward(float speed);
  void moveUp(float speed);

  // Perspective config
  void setPerspectiveProjection(float fov, float aspect, float near, float far);
  void setAspectRatio(float aspect);

  // Accessors
  Transform *getTransform() { return transform; }
  Camera *getCamera() { return camera; }
  const glm::mat4 &getProjection() const { return camera->getProjection(); }
  const glm::mat4 &getView() const { return camera->getView(); }

private:
  GameObject *cameraObject = nullptr;
  Transform *transform = nullptr;
  Camera *camera = nullptr;
};

} // namespace Magma
