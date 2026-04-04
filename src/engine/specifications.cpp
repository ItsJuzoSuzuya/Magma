module; 
#include <cstdint>
#include <string>

module engine:specification;

namespace Magma {

export struct WindowSpecifiction{
  std::string name;
  uint32_t windowWidth;
  uint32_t windowHeight;
};

} // namespace Magma
