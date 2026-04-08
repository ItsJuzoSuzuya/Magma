module;
#include <string>

export module core:pipeline_shader_info;

namespace Magma {

export struct PipelineShaderInfo {
  std::string vertFile;
  std::string fragFile;
};

} // namespace Magma
