#pragma once
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/glm.hpp>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

namespace magma {

class Camera {
public:
  void setPerspectiveProjection(float fov, float aspect, float near, float far);
  void setView(const glm::vec3 &position, const glm::vec3 &rotaion);

  const glm::mat4 &getProjection() const { return projectionMatrix; }
  const glm::mat4 &getView() const { return viewMatrix; }

  void follow(const glm::vec3 &position, glm::vec3 &rotation,
              glm::vec3 offset = {0.f, 0.f, 0.f}) {
    setView(position + offset, rotation);
  }

  bool canSee(const glm::vec3 &position) const;

private:
  glm::mat4 projectionMatrix{1.f};
  glm::mat4 viewMatrix{1.f};
};
} // namespace magma
