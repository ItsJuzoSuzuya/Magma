#include "mesh.hpp"
#include "../core/device.hpp"
#include "../core/frame_info.hpp"
#include "../core/mesh_data.hpp"
#include <cstdint>
#include <memory>
#include <print>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/tiny_gltf.h"

using namespace std;
namespace Magma {
// Constructor and destructor
Mesh::Mesh(Device &device) : device{device} {}

Mesh::~Mesh() {
  if (meshData) {
    delete meshData;
    meshData = nullptr;
  }
}

// --- Public --- //
// Load mesh from file
bool Mesh::load(const string &filepath) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  string err;
  string warn;

  bool result = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath);

  if (!warn.empty())
    println("Warning: {}", warn);

  if (!err.empty())
    return false;

  if (!result) {
    println("Failed to load glTF model: {}", filepath);
    return false;
  } else {
    println("Loaded glTF model: {}", filepath);
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

      meshData = new MeshData();

      for (size_t i = 0; i < vertexAccessor.count; i++) {
        MeshData::Vertex vertex;
        vertex.position = {vertexData[i * 3], vertexData[i * 3 + 1],
                           vertexData[i * 3 + 2]};
        vertex.normal = {normalData[i * 3], normalData[i * 3 + 1],
                         normalData[i * 3 + 2]};
        meshData->vertices.push_back(vertex);
      }

      if (primitive.indices >= 0) {
        const auto &indexAccessor = gltfModel.accessors[primitive.indices];
        const auto &indexBufferView =
            gltfModel.bufferViews[indexAccessor.bufferView];
        const auto &indexBuffer = gltfModel.buffers[indexBufferView.buffer];
        const uint32_t *indexData = reinterpret_cast<const uint32_t *>(
            &indexBuffer.data[0] + indexBufferView.byteOffset +
            indexAccessor.byteOffset);

        for (size_t i = 0; i < indexAccessor.count; i++)
          meshData->indices.push_back(indexData[i]);
      }
    }
  }

  createVertexBuffer();
  createIndexBuffer();

  return true;
}

// Render
void Mesh::onRender(Renderer &renderer) {
  println("Mesh::onRender -> Binding Buffers");
  println("Has Index Buffer: {}", hasIndexBuffer);
  println("Vertex Count: {}", meshData->vertices.size());
  println("Index Count: {}", meshData->indices.size());
  println("Vertex Buffer: {}", (void *)vertexBuffer->getBuffer());
  println("Index Buffer: {}",
          hasIndexBuffer ? (void *)indexBuffer->getBuffer() : 0);
  VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(FrameInfo::commandBuffer, 0, 1, vertexBuffers,
                         offsets);

  if (hasIndexBuffer)
    vkCmdBindIndexBuffer(FrameInfo::commandBuffer, indexBuffer->getBuffer(), 0,
                         VK_INDEX_TYPE_UINT32);
}

void Mesh::draw() {
  println("Mesh::draw -> Drawing Mesh");
  if (hasIndexBuffer)
    vkCmdDrawIndexed(FrameInfo::commandBuffer,
                     static_cast<uint32_t>(meshData->indices.size()), 1, 0, 0,
                     0);
  else
    vkCmdDraw(FrameInfo::commandBuffer,
              static_cast<uint32_t>(meshData->vertices.size()), 1, 0, 0);
}

// --- Private --- //
// Buffers
void Mesh::createVertexBuffer() {
  assert(meshData != nullptr &&
         "Cannot create vertex buffer before loading mesh data!");

  uint32_t vertexCount = static_cast<uint32_t>(meshData->vertices.size());
  uint32_t vertexSize = sizeof(meshData->vertices[0]);
  uint32_t bufferSize = vertexSize * vertexCount;

  Buffer stagingBuffer(device, vertexSize, vertexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)meshData->vertices.data());

  vertexBuffer = make_unique<Buffer>(device, vertexSize, vertexCount,
                                     VK_BUFFER_USAGE_VERTEX_BUFFER_BIT |
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device.copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(),
                    bufferSize);
}

void Mesh::createIndexBuffer() {
  assert(meshData != nullptr &&
         "Cannot create index buffer before loading mesh data!");

  uint32_t indexCount = static_cast<uint32_t>(meshData->indices.size());
  hasIndexBuffer = indexCount > 0;

  if (!hasIndexBuffer)
    return;

  uint32_t indexSize = sizeof(meshData->indices[0]);
  uint32_t bufferSize = indexSize * indexCount;

  Buffer stagingBuffer(device, indexSize, indexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                           VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)meshData->indices.data());

  indexBuffer = make_unique<Buffer>(device, indexSize, indexCount,
                                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT |
                                        VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                    VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

  device.copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(),
                    bufferSize);
}

} // namespace Magma
