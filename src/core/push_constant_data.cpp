module core:push_constanc_data;
import std;

namespace Magma {

struct PushConstantData {
  glm::mat4 modelMatrix{1.f};
  uint32_t objectId = 0;
};
} // namespace Magma
