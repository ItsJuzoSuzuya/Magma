#pragma once

#include <glm/mat4x4.hpp>
namespace Magma {

struct PushConstantData {
  glm::mat4 modelMatrix{1.f};
};
} // namespace Magma
