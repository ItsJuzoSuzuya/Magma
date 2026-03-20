module;
#include <vulkan/vulkan_core.h>

module features:render_feature;
import std;

namespace Magma {

class RenderFeature {
public:
  virtual ~RenderFeature() = default;

  virtual void onResize(VkExtent2D newExtent) = 0;

  virtual void prepare(uint32_t imageIndex) = 0;
  virtual void pushColorAttachments(
      std::vector<VkRenderingAttachmentInfo> &colors,
      uint32_t imageIndex) {}
  virtual void finish(uint32_t imageIndex) = 0;

};

}
