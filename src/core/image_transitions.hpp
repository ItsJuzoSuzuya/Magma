#pragma once
#include <vulkan/vulkan_core.h>

namespace Magma {

struct ImageTransitionDescription {
  VkImageLayout newLayout;
  VkPipelineStageFlags srcStage;
  VkPipelineStageFlags dstStage;
  VkAccessFlags srcAccess;
  VkAccessFlags dstAccess;
  VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
};

struct ImageTransition {
  inline static constexpr ImageTransitionDescription UndefinedToColorOptimal = {
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT,
      .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccess = 0,
      .dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT,
  };
  inline static constexpr ImageTransitionDescription PresentToColorOptimal = {
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccess = 0,
      .dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };
  inline static constexpr ImageTransitionDescription ShaderReadToColorOptimal = {
      .newLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
      .srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      .dstStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .srcAccess = VK_ACCESS_SHADER_READ_BIT,
      .dstAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
  };
  inline static constexpr ImageTransitionDescription ColorOptimalToShaderRead = {
      .newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
      .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
      .srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccess = VK_ACCESS_SHADER_READ_BIT,
  };
  inline static constexpr ImageTransitionDescription ColorOptimalToPresent = {
      .newLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
      .srcStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
      .dstStage = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT,
      .srcAccess = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
      .dstAccess = 0,
  };
};

}
