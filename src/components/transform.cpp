#include "transform.hpp"
namespace magma {

glm::mat4 Transform::mat4() const {
  const float c1 = cos(rotation.y);
  const float s1 = sin(rotation.y);
  const float c2 = cos(rotation.x);
  const float s2 = sin(rotation.x);
  const float c3 = cos(rotation.z);
  const float s3 = sin(rotation.z);
  return glm::mat4{{
                       scale.x * (c1 * c3 + s1 * s2 * s3),
                       scale.x * c2 * s3,
                       scale.x * (c1 * s2 * s3 - s1 * c3),
                       0.f,
                   },
                   {
                       scale.y * (s1 * s2 * c3 - s3 * c1),
                       scale.y * c2 * c3,
                       scale.y * (s1 * s3 + c1 * s2 * c3),
                       0.f,
                   },
                   {
                       scale.z * s1 * c2,
                       scale.z * -s2,
                       scale.z * c1 * c2,
                       0.f,
                   },
                   {position.x, position.y, position.z, 1.f}};
}

glm::mat4 Transform::normalMatrix() const {
  const float c1 = cos(rotation.y);
  const float s1 = sin(rotation.y);
  const float c2 = cos(rotation.x);
  const float s2 = sin(rotation.x);
  const float c3 = cos(rotation.z);
  const float s3 = sin(rotation.z);
  const glm::vec3 invScale = 1.f / scale;

  return glm::mat4{
      {
          invScale.x * (c1 * c3 + s1 * s2 * s3),
          invScale.x * c2 * s3,
          invScale.x * (c1 * s2 * s3 - s1 * c3),
          0.f,
      },
      {
          invScale.y * (s1 * s2 * c3 - s3 * c1),
          invScale.y * c2 * c3,
          invScale.y * (s1 * s3 + c1 * s2 * c3),
          0.f,
      },
      {
          invScale.z * s1 * c2,
          invScale.z * -s2,
          invScale.z * c1 * c2,
          0.f,
      },
      {0.f, 0.f, 0.f, 1.f},
  };
}
} // namespace magma
