#pragma once
#include <stdexcept>
#include <vulkan/vulkan_core.h>

namespace Magma {

class RenderTarget {
public:
  virtual ~RenderTarget() = default;

  // No Copy 
  RenderTarget(const RenderTarget &) = delete;
  RenderTarget &operator=(const RenderTarget &) = delete;

  // Common Interface 
  virtual VkRenderPass getRenderPass() const = 0;
  virtual VkFramebuffer getFrameBuffer(int index) const = 0;
  virtual VkExtent2D extent() const = 0;

  // Offscreen-only helpers (default: no sampler / no image)
  virtual VkImage &getColorImage(int index) = 0;
  virtual VkImageView getColorImageView(int index) const = 0;
  virtual VkSampler getColorSampler() const = 0;
  virtual VkFormat getColorFormat() const = 0;
  virtual VkFormat getDepthFormat() const = 0;

  virtual uint32_t getColorAttachmentCount() const { return 1; }

  // Resizing - implementations decide what overload(s) they support
  virtual void resize(VkExtent2D newExtent) { }
  virtual void resize(VkExtent2D newExtent, VkSwapchainKHR swapChain) { }

  virtual void cleanup() = 0;

protected:
  RenderTarget() = default;

};
} // namespace Magma
