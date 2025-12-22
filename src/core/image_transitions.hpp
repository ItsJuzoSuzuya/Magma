#pragma once
#include <vulkan/vulkan_core.h>

namespace Magma {

struct ImageTransitionDescription {
  VkImageLayout oldLayout;
  VkImageLayout newLayout;
  VkPipelineStageFlags srcStage;
  VkPipelineStageFlags dstStage;
  VkAccessFlags srcAccess;
  VkAccessFlags dstAccess;
  VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
};

struct ImageTransition {
  inline static constexpr ImageTransitionDescription UndefinedToColorOptimal = {
      .oldLayout = VK_IMAGE_LAYOUT_UNDEFINED,
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccess = 0,
      .dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT,
  };

};

}
