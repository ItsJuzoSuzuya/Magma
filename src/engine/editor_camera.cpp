module engine:editor_camera;
import std;

export namespace Magma {

/**
 * EditorCamera aggregates a Transform and a Camera for editor usage.
 * It owns the transform and camera instances and exposes simple movement
 * APIs for the editor (e.g., when user presses WASD in the offscreen view).
 */
export class EditorCamera{
public:
  EditorCamera(){
    cameraObject = new GameObject(UINT32_MAX, "Editor Camera");
    transform = cameraObject->addComponent<Transform>();
    camera = cameraObject->addComponent<Camera>();
    camera->setPerspectiveProjection(glm::radians(90.f), 16.f / 9.f, 0.1f, 100.f);
  }

  ~EditorCamera() = default;

  void EditorCamera::onUpdate() {
    camera->onUpdate();
  }
  void EditorCamera::onRender(SceneRenderer &renderer) {
    camera->onRender(renderer);
  }

  // Movement helpers used by editor input
  void EditorCamera::moveRight(float speed) {
    transform->position += transform->right() * speed;
  }

  void EditorCamera::moveForward(float speed) {
    transform->position += transform->forward() * speed;
  }

  void EditorCamera::moveUp(float speed) {
    transform->position += transform->up() * speed;
  }

  // Perspective config
  void EditorCamera::setPerspectiveProjection(float fov, float aspect, float near,
                                              float far) {
    camera->setPerspectiveProjection(fov, aspect, near, far);
  }

  void EditorCamera::setAspectRatio(float aspect) {
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
