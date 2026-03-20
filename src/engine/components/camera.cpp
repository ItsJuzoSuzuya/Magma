module;
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/trigonometric.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

module components:camera;

namespace Magma {

export struct CameraUBO {
  glm::mat4 projectionView{1.f};
};

export struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

export class Camera : public Component {
public:
  Camera(GameObject *owner): Component(owner), 
    ownerTransform(owner->getComponent<Transform>()) {}

  void setPerspectiveProjection(float fov, float aspect, float near, float far){
    this->fov = glm::clamp(fov, 0.01f, glm::radians(179.f));
    aspectRatio = aspect;
    nearPlane = near;
    farPlane = far;

    calculateProjectionMatrix();
  }

  void setFOV(float fov) {
    this->fov = glm::clamp(fov, 0.01f, glm::radians(179.f));
    calculateProjectionMatrix();
  }

  void setAspectRatio(float aspect) {
    aspectRatio = aspect;
    calculateProjectionMatrix();
  }

  void onUpdate() {
    if (!ownerTransform)
      return;

    setView(ownerTransform->position, ownerTransform->rotation);
  }

  void setView(const glm::vec3 &position, const glm::vec3 &rotation) {
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const glm::vec3 u{c1 * c3 + s1 * s2 * s3, c2 * s3, c1 * s2 * s3 - s1 * c3};
    const glm::vec3 v{s1 * s2 * c3 - s3 * c1, c2 * c3, s1 * s3 + c1 * s2 * c3};
    const glm::vec3 w{s1 * c2, -s2, c1 * c2};

    viewMatrix = glm::mat4{1.f};
    viewMatrix[0][0] = u.x;
    viewMatrix[1][0] = u.y;
    viewMatrix[2][0] = u.z;
    viewMatrix[0][1] = v.x;
    viewMatrix[1][1] = v.y;
    viewMatrix[2][1] = v.z;
    viewMatrix[0][2] = w.x;
    viewMatrix[1][2] = w.y;
    viewMatrix[2][2] = w.z;
    viewMatrix[3][0] = -glm::dot(u, position);
    viewMatrix[3][1] = -glm::dot(v, position);
    viewMatrix[3][2] = -glm::dot(w, position);
  };

  bool canSee(const glm::vec3 &position) const {
    AABB chunkBounds;
    chunkBounds.min = position;
    chunkBounds.max = position + glm::vec3{32.f, 32.f, 32.f};

    std::array<glm::vec3, 8> corners;
    corners[0] = {chunkBounds.min};
    corners[1] = {chunkBounds.min.x, chunkBounds.min.y, chunkBounds.max.z};
    corners[2] = {chunkBounds.min.x, chunkBounds.max.y, chunkBounds.min.z};
    corners[3] = {chunkBounds.min.x, chunkBounds.max.y, chunkBounds.max.z};
    corners[4] = {chunkBounds.max.x, chunkBounds.min.y, chunkBounds.min.z};
    corners[5] = {chunkBounds.max.x, chunkBounds.min.y, chunkBounds.max.z};
    corners[6] = {chunkBounds.max.x, chunkBounds.max.y, chunkBounds.min.z};
    corners[7] = {chunkBounds.max};

    for (const auto &corner : corners) {
      glm::vec4 clipSpace =
          projectionMatrix * viewMatrix * glm::vec4(corner, 1.f);

      bool inFrustum = (-clipSpace.w < clipSpace.x && clipSpace.x < clipSpace.w &&
                        -clipSpace.w < clipSpace.y && clipSpace.y < clipSpace.w &&
                        0 < clipSpace.z && clipSpace.z < clipSpace.w);
      if (inFrustum)
        return true;
    }
    return false;
  }

  // --- Lifecycle ---
  void onRender(SceneRenderer &renderer) {
    if (ownerTransform)
      setView(ownerTransform->position, ownerTransform->rotation);

    CameraUBO ubo{};
    ubo.projectionView = getProjection() * getView();
    

    renderer.uploadCameraUBO(ubo);
  }

#if defined(MAGMA_WITH_EDITOR)
  void onInspector() {
    GameObject *activeCamera = Scene::getActiveCamera();
    bool isActive = (activeCamera == owner);
    if(ImGui::Checkbox("Active Camera", &isActive)) {
      if(isActive) 
        Scene::setActiveCamera(owner);
      else 
        Scene::setActiveCamera(nullptr);
    }

    ImGui::Separator();
    ImGui::Text("Projection Settings");
    float fovDegrees = glm::degrees(fov);
    if (ImGui::SliderFloat("FOV", &fovDegrees, 1.f, 179.f)) {
      setFOV(glm::radians(fovDegrees));
    }
  }

  const char *inspectorName() const override { return "Camera"; }
  const float inspectorHeight() const override { return 150.0f; }
#endif

private:
  Transform *ownerTransform = nullptr;
  SceneRenderer *targetRenderer = nullptr;

  float fov;
  float aspectRatio;
  float nearPlane;
  float farPlane;

  glm::mat4 projectionMatrix{1.f};
  void calculateProjectionMatrix() {
    float tanHalfFov = tan(fov / 2.f);
    projectionMatrix = glm::mat4(0.f);
    projectionMatrix[0][0] = 1.f / (aspectRatio * tanHalfFov);
    projectionMatrix[1][1] = 1.f / tanHalfFov;
    projectionMatrix[2][2] = farPlane / (farPlane - nearPlane);
    projectionMatrix[2][3] = 1.f;
    projectionMatrix[3][2] = -(farPlane * nearPlane) / (farPlane - nearPlane);
  }

  glm::mat4 viewMatrix{1.f};

  void pushCameraDataToGPU(Buffer *uboBuffer) {
    CameraUBO ubo{};
    ubo.projectionView = getProjection() * getView();
    uboBuffer->writeToBuffer((void *)&ubo);
    uboBuffer->flush();
  }
};

} // namespace Magma
