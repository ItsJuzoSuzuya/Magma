#include "mesh.hpp"
#include <cstdint>
#include <iostream>
#include <sys/types.h>
#include <vulkan/vulkan_core.h>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/tiny_gltf.h"

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
  /*
  attributeDescriptions.push_back(
      {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position)});
  attributeDescriptions.push_back(
      {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, color)});
  attributeDescriptions.push_back(
      {2, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal)});
  attributeDescriptions.push_back(
      {3, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, texCoord)});
  */

  return attributeDescriptions;
}

bool Mesh::load(const string &filepath) {
  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  string err;
  string warn;

  bool result = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath);

  if (!warn.empty())
    cout << "Warning: " << warn << endl;

  if (!err.empty())
    return false;

  if (!result) {
    cout << "Failed to load glTF model!" << endl;
    return false;
  } else {
    cout << "Loaded glTF model: " << filepath << endl;
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
        vertex.texCoord = {uvData[i * 2], uvData[i * 2 + 1]};
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

        for (size_t i = 0; i < indexAccessor.count; i++) {
          meshData->indices.push_back(indexData[i]);
        }
      }
    }
  }
  return true;
}

} // namespace Magma
