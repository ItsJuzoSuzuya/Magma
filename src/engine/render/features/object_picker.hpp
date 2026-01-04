#pragma once

#include "engine/gameobject.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>
namespace Magma {

class RenderTargetInfo;
class ImageTransitionDescription;

class ObjectPicker {
public:
  ObjectPicker(VkExtent2D extent, uint32_t imageCount);

  VkImage getIdImage(uint32_t imageIndex) const {
    return idImages[imageIndex]; }
  VkImageView getIdImageView(uint32_t imageIndex) const {
    return idImageViews[imageIndex]; }
  VkImageLayout getIdImageLayout(uint32_t imageIndex) const {
    return idImageLayouts[imageIndex]; }
  VkRenderingAttachmentInfo getIdAttachment(uint32_t imageIndex) const;

  void transitionIdImage(size_t index,
                         ImageTransitionDescription transition);

  void requestPick(uint32_t x, uint32_t y);
  GameObject *pollPickResult();
  void servicePendingPick(); // executes after offscreen rendering

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

    GameObject *pickAtPixel(uint32_t x, uint32_t y);
  };
}
