#ifndef MODEL_HPP
#define MODEL_HPP
#include "../components/component.hpp"
#include "buffer.hpp"
#include "device.hpp"
#include <cstdint>
#include <glm/ext/vector_float2.hpp>
#include <glm/ext/vector_float3.hpp>
#include <memory>
#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>

using namespace std;
namespace magma {

class Model : public Component {
public:
  struct Vertex {
    glm::vec3 position{0.f, 0.f, 0.f};
    glm::vec3 color{0.f, 0.f, 0.f};
    glm::vec3 normal{0.f, 0.f, 0.f};
    glm::vec2 texCoord{0.f, 0.f};

    static vector<VkVertexInputBindingDescription> getBindingDescriptions();
    static vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
  };

  struct TexCoord {
    constexpr static glm::vec2 first = glm::vec2{0.f, 0.f};
    constexpr static glm::vec2 second = glm::vec2{0.f, 1.f};
    constexpr static glm::vec2 third = glm::vec2{1.f, 0.f};
    constexpr static glm::vec2 fourth = glm::vec2{1.f, 1.f};
  };

  struct Builder {
    vector<Vertex> vertices;
    vector<uint32_t> indices;

    void loadModel(const string &filepath);
    Builder &appendModel(const vector<Vertex> &vertices,
                         const vector<uint32_t> &indices);
  };

  Model(GameObject *parent, Device &device, const string &filepath);
  Model(GameObject *parent, Device &device, const Model::Builder &builder);
  Model(GameObject *parent, Device &device, const vector<Vertex> &vertices);
  Model(GameObject *parent, Device &device, const vector<Vertex> &vertices,
        const vector<uint32_t> &indices);

  Model(const Model &) = delete;
  Model &operator=(const Model &) = delete;

  shared_ptr<Buffer> vertexBuffer;
  shared_ptr<Buffer> indexBuffer;

  uint32_t vertexCount;
  uint32_t indexCount;

  void bind(VkCommandBuffer commandBuffer);
  void draw(VkCommandBuffer commandBuffer, uint32_t instanceCount = 1);

  static shared_ptr<Model> createModelFromFile(GameObject *parent,
                                               Device &device,
                                               const string &filepath);
  static unique_ptr<Model>
  loadFromMeshes(GameObject *parent, Device &device,
                 vector<pair<vector<Model::Vertex>, vector<uint32_t>>> &meshes);

private:
  Device &device;

  bool hasIndexBuffer;

  void createVertexBuffer(uint32_t size);
  void createIndexBuffer(uint32_t size);
  void createVertexBuffer(const vector<Vertex> &vertices);
  void createIndexBuffer(const vector<uint32_t> &indices);
  void createStagingBuffers(uint32_t vertexCount, uint32_t indexCount);
  void createRingBuffer(uint32_t vertexCount, uint32_t indexCount);
};
} // namespace magma
#endif
