#pragma once
#include "mesh_data.hpp"
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace Magma {

std::vector<VkVertexInputBindingDescription>
MeshData::Vertex::getBindingDescriptions() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(Vertex);
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
MeshData::Vertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
  attributeDescriptions.push_back(
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});

  return attributeDescriptions;
}

} // namespace Magma
