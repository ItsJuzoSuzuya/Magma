#include "pipeline.hpp"
#include "device.hpp"
#include "mesh_data.hpp"
#include <cassert>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iostream>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

Pipeline::Pipeline(Device &device, const std::string &vertFilepath,
                   const std::string &fragFilepath,
                   const PipelineConfigInfo &configInfo)
    : device{device} {
  createGraphicsPipeline(vertFilepath, fragFilepath, configInfo);
}

Pipeline::~Pipeline() {
  vkDestroyShaderModule(device.device(), vertShaderModule, nullptr);
  vkDestroyShaderModule(device.device(), fragShaderModule, nullptr);
  vkDestroyPipeline(device.device(), graphicsPipeline, nullptr);
}

void Pipeline::bind(VkCommandBuffer commandBuffer) {
  vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS,
                    graphicsPipeline);
}

void Pipeline::createGraphicsPipeline(const std::string &vertFilepath,
                                      const std::string &fragFilepath,
                                      const PipelineConfigInfo &configInfo) {
  assert(configInfo.pipelineLayout != VK_NULL_HANDLE &&
         "Cannot create graphics pipeline. Invalid pipeline layout!");
  assert(configInfo.renderPass != VK_NULL_HANDLE &&
         "Cannot create graphics pipeline. Invalid render pass!");

  std::vector<char> vertCode = readFile(vertFilepath);
  std::vector<char> fragCode = readFile(fragFilepath);

  createShaderModule(vertCode, &vertShaderModule);
  createShaderModule(fragCode, &fragShaderModule);

  VkPipelineShaderStageCreateInfo shaderStages[2];
  shaderStages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
  shaderStages[0].module = vertShaderModule;
  shaderStages[0].pName = "main";
  shaderStages[0].flags = 0;
  shaderStages[0].pNext = nullptr;
  shaderStages[0].pSpecializationInfo = nullptr;

  shaderStages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
  shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
  shaderStages[1].module = fragShaderModule;
  shaderStages[1].pName = "main";
  shaderStages[1].flags = 0;
  shaderStages[1].pNext = nullptr;
  shaderStages[1].pSpecializationInfo = nullptr;

  auto bindingDescriptions = MeshData::Vertex::getBindingDescriptions();
  auto attributeDescriptions = MeshData::Vertex::getAttributeDescriptions();

  VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
  vertexInputInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
  vertexInputInfo.vertexBindingDescriptionCount =
      static_cast<uint32_t>(bindingDescriptions.size());
  vertexInputInfo.vertexAttributeDescriptionCount =
      static_cast<uint32_t>(attributeDescriptions.size());
  vertexInputInfo.pVertexBindingDescriptions = bindingDescriptions.data();
  vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

  VkGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
  pipelineInfo.stageCount = 2;
  pipelineInfo.pStages = shaderStages;
  pipelineInfo.pVertexInputState = &vertexInputInfo;
  pipelineInfo.pInputAssemblyState = &configInfo.inputAssemblyInfo;
  pipelineInfo.pViewportState = &configInfo.viewportInfo;
  pipelineInfo.pRasterizationState = &configInfo.rasterizationInfo;
  pipelineInfo.pMultisampleState = &configInfo.multisampleInfo;
  pipelineInfo.pDepthStencilState = &configInfo.depthStencilInfo;
  pipelineInfo.pColorBlendState = &configInfo.colorBlendInfo;
  pipelineInfo.pDynamicState = &configInfo.dynamicStateInfo;

  pipelineInfo.layout = configInfo.pipelineLayout;
  pipelineInfo.renderPass = configInfo.renderPass;
  pipelineInfo.subpass = configInfo.subpass;

  pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
  pipelineInfo.basePipelineIndex = -1;

  pipelineInfo.pNext = nullptr;

  if (vkCreateGraphicsPipelines(device.device(), VK_NULL_HANDLE, 1,
                                &pipelineInfo, nullptr,
                                &graphicsPipeline) != VK_SUCCESS)
    throw std::runtime_error("Failed to create graphics pipeline!");
}

std::vector<char> Pipeline::readFile(const std::string &path) {
  std::ifstream file{path, std::ios::ate | std::ios::binary};

  if (!file.is_open())
    throw std::runtime_error("Failed to open File: " + path);

  size_t fileSize = static_cast<size_t>(file.tellg());
  std::vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();
  return buffer;
}

void Pipeline::createShaderModule(const std::vector<char> &code,
                                  VkShaderModule *shaderModule) {
  VkShaderModuleCreateInfo createInfo = {};
  createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
  createInfo.codeSize = code.size();
  createInfo.pCode = reinterpret_cast<const uint32_t *>(code.data());

  if (vkCreateShaderModule(device.device(), &createInfo, nullptr,
                           shaderModule) != VK_SUCCESS)
    throw std::runtime_error("Failed to create shader module!");
}

void Pipeline::defaultPipelineConfig(PipelineConfigInfo &configInfo) {
  configInfo.dynamicStates = {VK_DYNAMIC_STATE_VIEWPORT,
                              VK_DYNAMIC_STATE_SCISSOR};
  // Dynamic States
  configInfo.dynamicStateInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
  configInfo.dynamicStateInfo.dynamicStateCount =
      static_cast<uint32_t>(configInfo.dynamicStates.size());
  configInfo.dynamicStateInfo.pDynamicStates = configInfo.dynamicStates.data();
  configInfo.dynamicStateInfo.flags = 0;

  // Viewport (dynamic state)
  configInfo.viewportInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
  configInfo.viewportInfo.viewportCount = 1;
  configInfo.viewportInfo.pViewports = nullptr;
  configInfo.viewportInfo.scissorCount = 1;
  configInfo.viewportInfo.pScissors = nullptr;

  // Input Assembler
  configInfo.inputAssemblyInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
  configInfo.inputAssemblyInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
  configInfo.inputAssemblyInfo.primitiveRestartEnable = VK_FALSE;

  configInfo.tessellationInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO;
  configInfo.tessellationInfo.patchControlPoints = 3;

  // Rasterizer
  configInfo.rasterizationInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
  configInfo.rasterizationInfo.depthClampEnable = VK_FALSE;
  configInfo.rasterizationInfo.rasterizerDiscardEnable = VK_FALSE;
  configInfo.rasterizationInfo.polygonMode = VK_POLYGON_MODE_FILL;
  configInfo.rasterizationInfo.lineWidth = 1.0f;
  configInfo.rasterizationInfo.cullMode = VK_CULL_MODE_NONE;
  configInfo.rasterizationInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
  configInfo.rasterizationInfo.depthBiasEnable = VK_FALSE;

  // Multisampling
  configInfo.multisampleInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
  configInfo.multisampleInfo.sampleShadingEnable = VK_FALSE;
  configInfo.multisampleInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

  // Depth and Stencil Test
  configInfo.depthStencilInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
  configInfo.depthStencilInfo.depthTestEnable = VK_TRUE;
  configInfo.depthStencilInfo.depthWriteEnable = VK_TRUE;
  configInfo.depthStencilInfo.depthCompareOp = VK_COMPARE_OP_LESS;
  configInfo.depthStencilInfo.depthBoundsTestEnable = VK_FALSE;
  configInfo.depthStencilInfo.stencilTestEnable = VK_FALSE;

  // Color Blending
  configInfo.colorBlendAttachment.colorWriteMask =
      VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT |
      VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
  configInfo.colorBlendAttachment.blendEnable = VK_FALSE;

  configInfo.colorBlendInfo.sType =
      VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
  configInfo.colorBlendInfo.logicOpEnable = VK_FALSE;
  configInfo.colorBlendInfo.attachmentCount = 1;
  configInfo.colorBlendInfo.pAttachments = &configInfo.colorBlendAttachment;
}
} // namespace Magma
