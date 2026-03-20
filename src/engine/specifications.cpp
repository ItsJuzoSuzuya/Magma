module; 
#include <cstdint>

module engine:specification;
import std;

namespace Magma {

export struct EngineSpecifications {
  std::string name;
  uint32_t windowWidth;
  uint32_t windowHeight;
};

} // namespace Magma
