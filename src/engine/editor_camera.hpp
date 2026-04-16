#pragma once
#include "engine/components/camera.hpp"
#include "engine/components/transform.hpp"
#include "engine/gameobject.hpp"
#include <memory>

namespace Magma {

/**
 * Wraps a GameObject with Camera and Transform for editor viewport use.
 */
class EditorCamera {
public:
  EditorCamera() = default;
  EditorCamera(std::unique_ptr<GameObject> obj);
  EditorCamera(EditorCamera&&) = default;
  EditorCamera& operator=(EditorCamera&&) = default;

  void onUpdate();

  RenderProxy collectProxy() const;

  void setPerspectiveProjection(float fov, float aspect, float near, float far);
  void setAspectRatio(float aspect);

  void moveRight(float speed);
  void moveForward(float speed);
  void moveUp(float speed);

  const glm::mat4 &getProjection() const;
  const glm::mat4 &getView() const ;

  Transform *getTransform() { return transform; }
  Camera *getCamera() { return camera; }

private:
  std::unique_ptr<GameObject> cameraObject = nullptr;
  Transform *transform = nullptr;
  Camera *camera = nullptr;
};

} // namespace Magma
