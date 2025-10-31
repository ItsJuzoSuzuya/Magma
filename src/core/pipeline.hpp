#pragma once
#include <cstdint>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Device;

struct PipelineConfigInfo {
  PipelineConfigInfo() = default;
  PipelineConfigInfo(const PipelineConfigInfo &) = delete;
  PipelineConfigInfo &operator=(const PipelineConfigInfo &) = delete;

  VkPipelineViewportStateCreateInfo viewportInfo;
  VkPipelineInputAssemblyStateCreateInfo inputAssemblyInfo;
  VkPipelineTessellationStateCreateInfo tessellationInfo;
  VkPipelineRasterizationStateCreateInfo rasterizationInfo;
  VkPipelineMultisampleStateCreateInfo multisampleInfo;
  std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
  VkPipelineColorBlendStateCreateInfo colorBlendInfo;
  VkPipelineDepthStencilStateCreateInfo depthStencilInfo;
  std::vector<VkDynamicState> dynamicStates;
  VkPipelineDynamicStateCreateInfo dynamicStateInfo;
  VkPipelineLayout pipelineLayout = nullptr;
  VkRenderPass renderPass = nullptr;
  uint32_t subpass = 0;
};

class Pipeline {
public:
  Pipeline(const std::string &vertFilepath,
           const std::string &fragFilepath,
           const PipelineConfigInfo &configInfo);
  ~Pipeline();

  Pipeline(const Pipeline &) = delete;
  Pipeline &operator=(const Pipeline &) = delete;

  static void defaultPipelineConfig(PipelineConfigInfo &configInfo);

  void bind(VkCommandBuffer commandBuffer);

private:
  VkPipeline graphicsPipeline;
  VkShaderModule vertShaderModule;
  VkShaderModule tcsShaderModule;
  VkShaderModule tesShaderModule;
  VkShaderModule fragShaderModule;

  void createGraphicsPipeline(const std::string &vertFilepath,
                              const std::string &fragFilepath,
                              const PipelineConfigInfo &configInfo);
  std::vector<char> readFile(const std::string &filepath);
  void createShaderModule(const std::vector<char> &code,
                          VkShaderModule *shaderModule);
};
} // namespace Magma
