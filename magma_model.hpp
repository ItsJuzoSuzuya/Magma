#include "magma_device.hpp"
#include <cstdint>
#include <glm/fwd.hpp>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>

namespace magma {

class MagmaModel {
public:
  struct Vertex {
    glm::vec3 position{};
    glm::vec3 color{};
    glm::vec3 normal{};
    glm::vec2 uv{};

    static std::vector<VkVertexInputBindingDescription>
    getBindingDescriptions();
    static std::vector<VkVertexInputAttributeDescription>
    getAttributeDescriptions();

    bool operator==(const Vertex &other) const {
      return position == other.position && color == other.color &&
             normal == other.normal && uv == other.uv;
    }
  };

  struct Builder {
    std::vector<Vertex> vertices{};
    std::vector<uint32_t> indices{};

    void loadModel(const std::string &filepath);
  };

  MagmaModel(MagmaDevice &device, const MagmaModel::Builder &builder);
  ~MagmaModel();

  MagmaModel(const MagmaModel &) = delete;
  MagmaModel &operator=(const MagmaModel &) = delete;

  static std::unique_ptr<MagmaModel>
  createModelFromFile(MagmaDevice &device, const std::string &filepath);

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer);

private:
  void createVertexBuffers(const std::vector<Vertex> &vertices);
  void createIndexBuffers(const std::vector<uint32_t> &indices);

  MagmaDevice &magmaDevice;

  VkBuffer vertexBuffer;
  VkDeviceMemory vertexBufferMemory;
  uint32_t vertexCount;

  bool hasIndexBuffer = false;
  VkBuffer indexBuffer;
  VkDeviceMemory indexBufferMemory;
  uint32_t indexCount;
};

} // namespace magma
