#include "renderer.hpp"
#include "mesh.hpp"
#include "swapchain.hpp"
#include <cassert>
#include <glm/ext/scalar_constants.hpp>
#include <vulkan/vulkan_core.h>

using namespace std;

namespace Magma {

// Constructor and destructor
Renderer::Renderer(Device &device, VkDescriptorSetLayout descriptorSetLayout)
    : device{device} {
  createPipelineLayout(descriptorSetLayout);
}

Renderer::~Renderer() {
  vkDestroyPipelineLayout(device.device(), pipelineLayout, nullptr);
}

// --- Private ---
// Pipeline

void Renderer::createPipelineLayout(VkDescriptorSetLayout descriptorSetLayout) {

  vector<VkDescriptorSetLayout> layouts{descriptorSetLayout};

  VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
  pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
  pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
  pipelineLayoutInfo.pSetLayouts = layouts.data();

  if (vkCreatePipelineLayout(device.device(), &pipelineLayoutInfo, nullptr,
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
      make_unique<Pipeline>(device, "src/shaders/shader.vert.spv",
                            "src/shaders/shader.frag.spv", piplineConfigInfo);
}

} // namespace Magma
