module;
#include <vulkan/vulkan_core.h>
#include <glm/vec3.hpp>

module core:mesh_data;

namespace Magma {

export struct MeshData {
  struct Vertex {
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::vec3 color{0.f, 0.f, 0.f};
    glm::vec3 normal{0.f, 0.f, 0.f};

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions() {
      std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
      bindingDescriptions[0].binding = 0;
      bindingDescriptions[0].stride = sizeof(Vertex);
      bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
      return bindingDescriptions;
    }
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions() {
      std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
      attributeDescriptions.push_back(
          {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
      attributeDescriptions.push_back(
          {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
      return attributeDescriptions;
    }
  };

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  uint32_t vertexOffset = 0;
  uint32_t indexOffset = 0;
};

} // namespace Magma
