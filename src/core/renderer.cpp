#include "renderer.hpp"
#include "mesh_data.hpp"
#include "push_constant_data.hpp"
#include "render_system.hpp"
#include "swapchain.hpp"
#include <cassert>
#include <glm/ext/scalar_constants.hpp>
#include <vulkan/vulkan_core.h>

using namespace std;

namespace Magma {

// Constructor and destructor
void Renderer::init(VkDescriptorSetLayout descriptorSetLayout) {
  createPipelineLayout(descriptorSetLayout);
}

Renderer::~Renderer() {
  VkDevice device = Device::get().device();
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  pipeline = nullptr;
}

// --- Private --- //
// Pipeline Layout
void Renderer::createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout) {
  vector<VkDescriptorSetLayout> layouts{descriptorSetLayout};

  VkPushConstantRange pushConstantRange = {};
  pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
  pushConstantRange.offset = 0;
  pushConstantRange.size = sizeof(PushConstantData);

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();
  pipelineLayoutInfo.pushConstantRangeCount = 1;
  pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

  VkDevice device = Device::get().device();
  if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                             &pipelineLayout) != VK_SUCCESS)
    throw runtime_error("Failed to create pipeline layout!");
}

void Renderer::createPipeline() {
  assert(pipelineLayout != nullptr &&
         "Cannot create pipeline before pipeline layout!");

  PipelineConfigInfo piplineConfigInfo = {};
  Pipeline::defaultPipelineConfig(piplineConfigInfo);
  piplineConfigInfo.renderPass = renderTarget->getRenderPass();
  piplineConfigInfo.pipelineLayout = pipelineLayout;

  pipeline =
      make_unique<Pipeline>("src/shaders/shader.vert.spv",
                            "src/shaders/shader.frag.spv", piplineConfigInfo);
}

} // namespace Magma
