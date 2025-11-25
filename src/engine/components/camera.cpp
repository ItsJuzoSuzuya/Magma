#include "camera.hpp"
#include "transform.hpp"
#include "../core/renderer.hpp"
#include "../core/buffer.hpp"
#include "../gameobject.hpp"
#include <array>
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>
#include <print>
#include <vulkan/vulkan_core.h>

namespace Magma {

struct AABB {
  glm::vec3 min;
  glm::vec3 max;
};

Camera::Camera(GameObject *owner)
    : Component(owner), ownerTransform(owner->getComponent<Transform>()) {}

void Camera::setPerspectiveProjection(float fov, float aspect, float near,
                                      float far) {
  float tanHalfFov = tan(fov / 2.f);
  projectionMatrix = glm::mat4(0.f);
  projectionMatrix[0][0] = 1.f / (aspect * tanHalfFov);
  projectionMatrix[1][1] = 1.f / tanHalfFov;
  projectionMatrix[2][2] = far / (far - near);
  projectionMatrix[2][3] = 1.f;
  projectionMatrix[3][2] = -(far * near) / (far - near);
}

void Camera::onUpdate() {
  if (!ownerTransform)
    return;

  setView(ownerTransform->position, ownerTransform->rotation);
}

void Camera::setView(const glm::vec3 &position,
                         const glm::vec3 &rotation) {
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

bool Camera::canSee(const glm::vec3 &position) const {
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

void Camera::pushCameraDataToGPU(Buffer *uboBuffer) {
    CameraUBO ubo{};
    ubo.projectionView = getProjection() * getView();
    uboBuffer->writeToBuffer((void *)&ubo);
    uboBuffer->flush();
}

// --- Lifecycle ---
void Camera::onRender(Renderer &renderer) {
  pushCameraDataToGPU(renderer.getCameraBuffer());
}

} // namespace magma
