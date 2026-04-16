#pragma once
#include "component.hpp"
#include "transform.hpp"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace Magma {

struct CameraUBO {
  glm::mat4 projectionView{1.f};
};

struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

class Camera : public Component {
public:
  Camera() : Component() {}

  void setPerspectiveProjection(float fov, float aspect, float near, float far);
  void setFOV(float fov);
  void setAspectRatio(float aspect);
  void setOwnerTransform(Transform *t) { ownerTransform = t; }

  const glm::mat4 &getProjection() const { return projectionMatrix; }
  const glm::mat4 &getView() const { return viewMatrix; }
  void setView(const glm::vec3 &position, const glm::vec3 &rotation);

  bool canSee(const glm::vec3 &position) const;

  void onUpdate() override;
  void collectProxy(RenderProxy &proxy) override;

  #if defined(MAGMA_WITH_EDITOR)
    void onInspector() override;
    const char *inspectorName() const override { return "Camera"; }
    const float inspectorHeight() const override { return 150.0f; }
  #endif

private:
  Transform *ownerTransform = nullptr;

  float fov = glm::radians(60.f);
  float aspectRatio = 16.f / 9.f;
  float nearPlane = 0.1f;
  float farPlane = 100.f;

  glm::mat4 projectionMatrix{1.f};
  void calculateProjectionMatrix();

  glm::mat4 viewMatrix{1.f};
};

} // namespace Magma
