module;
#include <cstdint>

module core:queue_family_indices;
import std;

namespace Magma {
export struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};
} // namespace Magma
