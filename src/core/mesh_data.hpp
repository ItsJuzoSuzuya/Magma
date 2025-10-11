#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class Device;

struct MeshData {
  struct Vertex {
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::vec3 color{0.f, 0.f, 0.f};
    glm::vec3 normal{0.f, 0.f, 0.f};

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();
  };

  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;

  uint32_t vertexOffset = 0;
  uint32_t indexOffset = 0;
};

} // namespace Magma
