#pragma once

#include "engine/gameobject.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
namespace Magma {

class RenderTargetInfo;

class ObjectPicker {
public:
  ObjectPicker(const RenderTargetInfo &info);

  VkImage getIdImage(uint32_t imageIndex) const {
    return idImages[imageIndex]; }
  VkImageView getIdImageView(uint32_t imageIndex) const {
    return idImageViews[imageIndex]; }
  VkRenderingAttachmentInfo getIdAttachment(uint32_t imageIndex) const;

  void requestPick(uint32_t x, uint32_t y);
  GameObject *pollPickResult();

private:
  uint32_t imageCount_ = 0;
  VkExtent2D targetExtent{};

  // Id image for object picking
  std::vector<VkImage> idImages;
  std::vector<VkDeviceMemory> idImageMemories;
  std::vector<VkImageView> idImageViews;
  std::vector<VkImageLayout> idImageLayouts;
  VkFormat idImageFormat = VK_FORMAT_R32_UINT;
  void createImages();
  void destroyImages();

  // Deferred Picking
  struct PendingPick {
    bool hasRequest = false;
    uint32_t x = 0, y = 0;
    GameObject *result = nullptr;
  } pendingPick;

    virtual GameObject *pickAtPixel(uint32_t x, uint32_t y);
    virtual void servicePendingPick(); // executes after offscreen rendering
  };
}
