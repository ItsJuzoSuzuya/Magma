module;
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>

module core:push_constanc_data;

namespace Magma {

struct PushConstantData {
  glm::mat4 modelMatrix{1.f};
  uint32_t objectId = 0;
};
} // namespace Magma
