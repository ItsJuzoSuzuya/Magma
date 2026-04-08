module;
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>

export module core:push_constant_data;

namespace Magma {

export struct PushConstantData {
  glm::mat4 modelMatrix{1.f};
  uint32_t objectId = 0;
};
} // namespace Magma
