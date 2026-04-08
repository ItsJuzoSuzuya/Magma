module; 
#include <cstdint>
#include <string>

export module core:specifications;

namespace Magma {

export struct WindowSpecification{
  std::string name;
  uint32_t windowWidth;
  uint32_t windowHeight;
};

} // namespace Magma
