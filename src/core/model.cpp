#include "model.hpp"
#include "device.hpp"
#include "swapchain.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <glm/ext/vector_float3.hpp>
#include <iostream>
#include <memory>
#include <utility>
#include <vector>
#include <vulkan/vulkan_core.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../include/tiny_gltf.h"

using namespace std;

namespace magma {

std::vector<VkVertexInputBindingDescription>
Model::Vertex::getBindingDescriptions() {
  std::vector<VkVertexInputBindingDescription> bindingDescriptions(1);
  bindingDescriptions[0].binding = 0;
  bindingDescriptions[0].stride = sizeof(Vertex);
  bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
  return bindingDescriptions;
}

std::vector<VkVertexInputAttributeDescription>
Model::Vertex::getAttributeDescriptions() {
  std::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
  attributeDescriptions.push_back(
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
  attributeDescriptions.push_back(
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
  attributeDescriptions.push_back(
      {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
  attributeDescriptions.push_back(
      {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)});

  return attributeDescriptions;
}

void Model::Builder::loadModel(const std::string &filepath) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  std::string err;
  std::string warn;

  bool result = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath);

  if (!warn.empty()) {
    std::cout << "Warning: " << warn << std::endl;
  }

  if (!err.empty()) {
    std::cout << "Error: " << err << std::endl;
  }

  if (!result) {
    std::cout << "Failed to load glTF model!" << std::endl;
    return;
  } else {
    std::cout << "Loaded glTF model: " << filepath << std::endl;
  }

  for (const auto &mesh : gltfModel.meshes) {
    for (const auto &primitive : mesh.primitives) {
      const auto &vertexAccessor =
          gltfModel.accessors[primitive.attributes.at("POSITION")];
      const auto &vertexBufferView =
          gltfModel.bufferViews[vertexAccessor.bufferView];
      const auto &vertexBuffer = gltfModel.buffers[vertexBufferView.buffer];
      const float *vertexData = reinterpret_cast<const float *>(
          &vertexBuffer
               .data[vertexBufferView.byteOffset + vertexAccessor.byteOffset]);

      const auto &normalAccessor =
          gltfModel.accessors[primitive.attributes.at("NORMAL")];
      const auto &normalBufferView =
          gltfModel.bufferViews[normalAccessor.bufferView];
      const auto &normalBuffer = gltfModel.buffers[normalBufferView.buffer];
      const float *normalData = reinterpret_cast<const float *>(
          &normalBuffer
               .data[normalBufferView.byteOffset + normalAccessor.byteOffset]);

      const auto &uvAccessor =
          gltfModel.accessors[primitive.attributes.at("TEXCOORD_0")];
      const auto &uvBufferView = gltfModel.bufferViews[uvAccessor.bufferView];
      const auto &uvBuffer = gltfModel.buffers[uvBufferView.buffer];
      const float *uvData = reinterpret_cast<const float *>(
          &uvBuffer.data[uvBufferView.byteOffset + uvAccessor.byteOffset]);

      for (size_t i = 0; i < vertexAccessor.count; i++) {
        Vertex vertex{};
        vertex.position = {vertexData[i * 3], vertexData[i * 3 + 1],
                           vertexData[i * 3 + 2]};
        vertex.normal = {normalData[i * 3], normalData[i * 3 + 1],
                         normalData[i * 3 + 2]};
        vertex.texCoord = {uvData[i * 2], uvData[i * 2 + 1]};
        vertices.push_back(vertex);
      }

      if (primitive.indices >= 0) {
        const auto &indexAccessor = gltfModel.accessors[primitive.indices];
        const auto &indexBufferView =
            gltfModel.bufferViews[indexAccessor.bufferView];
        const auto &indexBuffer = gltfModel.buffers[indexBufferView.buffer];

        const size_t indexCount = indexAccessor.count;
        const size_t offset =
            indexBufferView.byteOffset + indexAccessor.byteOffset;
        const unsigned char *dataPtr = indexBuffer.data.data() + offset;

        switch (indexAccessor.componentType) {
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
          const uint8_t *buf = reinterpret_cast<const uint8_t *>(dataPtr);
          for (size_t i = 0; i < indexCount; i++) {
            indices.push_back(static_cast<uint32_t>(buf[i]));
          }
          break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
          const uint16_t *buf = reinterpret_cast<const uint16_t *>(dataPtr);
          for (size_t i = 0; i < indexCount; i++) {
            indices.push_back(static_cast<uint32_t>(buf[i]));
          }
          break;
        }
        case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT: {
          const uint32_t *buf = reinterpret_cast<const uint32_t *>(dataPtr);
          for (size_t i = 0; i < indexCount; i++) {
            indices.push_back(buf[i]);
          }
          break;
        }
        default:
          std::cerr << "Unsupported index component type: "
                    << indexAccessor.componentType << std::endl;
          break;
        }
      }
    }
  }
}

Model::Builder &
Model::Builder::appendModel(const std::vector<Vertex> &vertices,
                            const std::vector<uint32_t> &indices) {
  this->vertices.insert(this->vertices.end(), vertices.begin(), vertices.end());
  this->indices.insert(this->indices.end(), indices.begin(), indices.end());

  return *this;
}

shared_ptr<Model> Model::createModelFromFile(GameObject *parent, Device &device,
                                             const std::string &filepath) {
  Builder builder = {};
  builder.loadModel(filepath);

  return std::make_shared<Model>(parent, device, builder);
}

std::unique_ptr<Model> Model::loadFromMeshes(
    GameObject *parent, Device &device,
    std::vector<std::pair<std::vector<Model::Vertex>, std::vector<uint32_t>>>
        &meshes) {

  Builder builder = {};
  for (auto &mesh : meshes) {
    builder.appendModel(mesh.first, mesh.second);
  }

  return std::make_unique<Model>(parent, device, builder);
}

Model::Model(GameObject *parent, Device &device, const string &filepath)
    : device{device} {
  createModelFromFile(parent, device, filepath);
}

Model::Model(GameObject *parent, Device &device, const Model::Builder &builder)
    : device{device} {
  setParent(parent);
  createVertexBuffer(builder.vertices);
  createIndexBuffer(builder.indices);
}

Model::Model(GameObject *parent, Device &device,
             const std::vector<Vertex> &vertices)
    : device{device} {
  setParent(parent);
  createVertexBuffer(vertices);
}

Model::Model(GameObject *parent, Device &device,
             const std::vector<Vertex> &vertices,
             const std::vector<uint32_t> &indices)
    : device{device} {
  setParent(parent);
  createVertexBuffer(vertices);
  createIndexBuffer(indices);
}

void Model::createVertexBuffer(const std::vector<Vertex> &vertices) {
  vertexCount = vertices.size();
  assert(vertexCount >= 3 && "Vertex count must be at least 3!");

  uint32_t vertexSize = sizeof(vertices[0]);
  VkDeviceSize bufferSize = vertexSize * vertexCount;

  Buffer stagingBuffer{device, vertexSize, vertexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)vertices.data());

  vertexBuffer = std::make_unique<Buffer>(device, vertexSize, vertexCount,
                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                              VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                          VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(),
                    bufferSize);
}

void Model::createIndexBuffer(const std::vector<uint32_t> &indices) {
  indexCount = static_cast<uint32_t>(indices.size());
  hasIndexBuffer = indexCount > 0;

  if (!hasIndexBuffer)
    return;

  uint32_t indexSize = sizeof(indices[0]);
  VkDeviceSize bufferSize = indexSize * indexCount;

  Buffer stagingBuffer{device, indexSize, indexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT};

  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)indices.data());

  indexBuffer = std::make_unique<Buffer>(device, indexSize, indexCount,
                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                             VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(),
                    bufferSize);
}

void Model::bind(VkCommandBuffer commandBuffer) {
  VkBuffer buffers[] = {vertexBuffer->getBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(commandBuffer, 0, 1, buffers, offsets);

  if (hasIndexBuffer)
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer->getBuffer(), 0,
                         VK_INDEX_TYPE_UINT32);
}

void Model::draw(VkCommandBuffer commandBuffer, uint32_t instanceCount) {
  if (hasIndexBuffer) {
    vkCmdDrawIndexed(commandBuffer, indexCount, instanceCount, 0, 0, 0);
  } else {
    vkCmdDraw(commandBuffer, vertexCount, 1, 0, 0);
  }
}
} // namespace magma
