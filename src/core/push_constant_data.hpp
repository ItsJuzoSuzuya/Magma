#pragma once

#include <glm/mat4x4.hpp>
namespace Magma {

struct PushConstantData {
  glm::mat4 modelMatrix{1.f};
  uint32_t objectId = 0;
};
} // namespace Magma
