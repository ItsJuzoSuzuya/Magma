module;
#include <vulkan/vulkan_core.h>

export module core:render_target;
import :image_transition;

namespace Magma {

export class IRenderTarget {
public:
  virtual ~IRenderTarget() = default;
  virtual void cleanup() = 0;

  // Common Interface
  virtual VkExtent2D extent() const = 0;
  virtual uint32_t imageCount() const = 0;

  virtual VkImage getColorImage(size_t index) const = 0;
  virtual VkImageView getColorImageView(size_t index) const = 0;
  virtual VkRenderingAttachmentInfo getColorAttachment(size_t index) const = 0;
  virtual uint32_t getColorAttachmentCount() const = 0;
  virtual VkImageLayout getColorImageLayout(size_t index) const = 0;
  virtual VkFormat getColorFormat() const = 0;
  virtual void transitionColorImage(size_t index, ImageTransitionDescription transition) = 0;

  virtual VkRenderingAttachmentInfo getDepthAttachment(size_t index) const = 0;
  virtual VkImageLayout getDepthImageLayout(size_t index) const = 0;
  virtual VkFormat getDepthFormat() const = 0;
  virtual void transitionDepthImage(size_t index, ImageTransitionDescription transition) = 0;
  
  virtual VkSampler getColorSampler() const = 0;

  virtual void onResize(const VkExtent2D newExtent) = 0;



};
} // namespace Magma
