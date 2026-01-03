#pragma once
#include "component.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace Magma {

class Transform;
class Buffer;

struct CameraUBO {
  glm::mat4 projectionView{1.f};
};

class Camera : public Component {
public:
  Camera(GameObject *owner);

  void setPerspectiveProjection(float fov, float aspect, float near, float far);
  void setFOV(float fov);
  void setAspectRatio(float aspect);

  const glm::mat4 &getProjection() const { return projectionMatrix; }
  const glm::mat4 &getView() const { return viewMatrix; }

  bool canSee(const glm::vec3 &position) const;

  // --- Lifecycle ---
  void onAwake() override {};
  void onUpdate() override;
  void onRender(SceneRenderer &renderer) override;

#if defined(MAGMA_WITH_EDITOR)
  // Inspector
  void onInspector() override;
  const char *inspectorName() const override { return "Camera"; }
  const float inspectorHeight() const override { return 150.0f; }
#endif

private:
  Transform *ownerTransform = nullptr;

  float fov;
  float aspectRatio;
  float nearPlane;
  float farPlane;

  glm::mat4 projectionMatrix{1.f};
  void calculateProjectionMatrix();

  glm::mat4 viewMatrix{1.f};
  void setView(const glm::vec3 &position, const glm::vec3 &rotaion);

  void pushCameraDataToGPU(Buffer *uboBuffer);
};
} // namespace Magma
