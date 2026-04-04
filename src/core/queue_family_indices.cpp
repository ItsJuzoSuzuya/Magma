module;
#include <cstdint>
#include <optional>

module core:queue_family_indices;

namespace Magma {
export struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;
  bool isComplete() {
    return graphicsFamily.has_value() && presentFamily.has_value();
  }
};
} // namespace Magma
