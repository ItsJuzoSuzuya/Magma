#include "magma_device.hpp"
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace magma {

class MagmaModel {
public:
  struct Vertex {
    glm::vec2 position;

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();
  };

  MagmaModel(MagmaDevice &device, const std::vector<Vertex> &vertices);
  ~MagmaModel();

  MagmaModel(const MagmaModel &) = delete;
  MagmaModel &operator=(const MagmaModel &) = delete;

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);

private:
  void createVertexBuffers(const std::vector<Vertex> &vertices);

  MagmaDevice &magmaDevice;
  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  uint32_t vertexCount;
};

} // namespace magma
