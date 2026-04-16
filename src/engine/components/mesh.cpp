#include "mesh.hpp"
#include "core/mesh_data.hpp"
#include "core/device.hpp"
#include "engine/scene.hpp"
#include "engine/scene_action.hpp"
#include "engine/scene_manager.hpp"
#include <algorithm>
#include <filesystem>
#include <print>

#define TINYGLTF3_IMPLEMENTATION
#define TINYGLTF3_ENABLE_FS          // enable file I/O
#define TINYGLTF3_ENABLE_STB_IMAGE   // enable image decoding
#include "tiny_gltf.h"

#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
  #include "core/window.hpp"
#endif

namespace fs = std::filesystem;

namespace Magma {

namespace string_utils {
inline std::string toLower(std::string str) {
  std::transform(str.begin(), str.end(), str.begin(),
            [](unsigned char c) { return (char)tolower(c); });
  return str;
}
inline bool hasAllowedExt(const fs::path &path) {
  if (!path.has_extension())
    return false;
  auto ext = toLower(path.extension().string());
  return ext == ".gltf";
}
} // namespace string_utils

Mesh::~Mesh() {
  if (meshData) {
    delete meshData;
    meshData = nullptr;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
  }
}

void Mesh::collectProxy(RenderProxy &proxy) {
  if (!meshData || !vertexBuffer)
    return;

  MeshProxy meshProxy = {};
  meshProxy.meshData = meshData;
  meshProxy.vertexBuffer = vertexBuffer->getBuffer();
  meshProxy.indexBuffer = indexBuffer ? indexBuffer->getBuffer() : VK_NULL_HANDLE;
  meshProxy.indexCount   = static_cast<uint32_t>(meshData->indices.size());
  meshProxy.vertexCount  = static_cast<uint32_t>(meshData->vertices.size());
  meshProxy.hasIndexBuffer = hasIndexBuffer;

  proxy.mesh = meshProxy;
}

#if defined(MAGMA_WITH_EDITOR)
bool Mesh::load() {
  if (sourcePath.empty()) {
    std::println("No source path set for mesh.");
    return false;
  }
  return load(sourcePath);
}
#endif

bool Mesh::load(const std::string &filepath) {
  if (meshData) {
    delete meshData;
    meshData = nullptr;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
  }

  tg3_parse_options opts;
  tg3_parse_options_init(&opts);
  tg3_error_stack errors = {};                                                                                                              
  tg3_model model = {};

  tg3_error_code result = tg3_parse_file(&model, &errors,
      filepath.c_str(), (uint32_t)filepath.size(), &opts);

  if (result != TG3_OK) {
    for (uint32_t i = 0; i < errors.count; i++) {
      const tg3_error_entry *e = tg3_errors_get(&errors, i);
      fprintf(stderr, "[%d] %s\n", (int)e->severity, e->message);
    }
    tg3_model_free(&model);
    return false;
  }
  std::println("Loading glTF model: {}", filepath);

  for (uint32_t mi = 0; mi < model.meshes_count; mi++) {
    const tg3_mesh &mesh = model.meshes[mi];
    for (uint32_t pi = 0; pi < mesh.primitives_count; pi++) {
      const tg3_primitive &prim= mesh.primitives[pi];

      int32_t posIdx = TG3_INDEX_NONE, normIdx = TG3_INDEX_NONE, uvIdx = TG3_INDEX_NONE, colorIdx = TG3_INDEX_NONE;
      for (uint32_t ai = 0; ai < prim.attributes_count; ai++) {
        const tg3_str &key = prim.attributes[ai].key;
        if (strncmp(key.data, "POSITION",    key.len) == 0) 
          posIdx = prim.attributes[ai].value;
        if (strncmp(key.data, "NORMAL",      key.len) == 0) 
          normIdx = prim.attributes[ai].value;
        if (strncmp(key.data, "TEXCOORD_0",  key.len) == 0) 
          uvIdx = prim.attributes[ai].value;
        if (strncmp(key.data, "COLOR_0",  key.len) == 0) 
          uvIdx = prim.attributes[ai].value;
      }

      if (posIdx == TG3_INDEX_NONE || normIdx == TG3_INDEX_NONE)
        continue;

      const tg3_accessor &posAcc  = model.accessors[posIdx];
      const tg3_accessor &normAcc = model.accessors[normIdx];
      const tg3_accessor &colorAcc = model.accessors[colorIdx];
      const tg3_buffer_view &posBV  = model.buffer_views[posAcc.buffer_view];
      const tg3_buffer_view &normBV = model.buffer_views[normAcc.buffer_view];
      const tg3_buffer_view &colorBV = model.buffer_views[colorAcc.buffer_view];
      const float *vertexData = reinterpret_cast<const float *>(
        model.buffers[posBV.buffer].data.data + 
        posBV.byte_offset + posAcc.byte_offset);
      const float *normalData = reinterpret_cast<const float *>(
        model.buffers[normBV.buffer].data.data +
        normBV.byte_offset + normAcc.byte_offset);
      const float *colorData = reinterpret_cast<const float *>(
        model.buffers[colorBV.buffer].data.data +
        colorBV.byte_offset + colorAcc.byte_offset);

      meshData = new MeshData();
        for (uint64_t i = 0; i < posAcc.count; i++) {
          MeshData::Vertex vertex;
          vertex.position = {vertexData[i*3], vertexData[i*3+1], vertexData[i*3+2]};
          vertex.normal   = {normalData[i*3], normalData[i*3+1], normalData[i*3+2]};
          vertex.color    = {colorData[i*3], colorData[i*3+1], colorData[i*3+2], colorData[i*3+3] };
          meshData->vertices.push_back(vertex);
        }

      if (prim.indices != TG3_INDEX_NONE) {
        const tg3_accessor   &idxAcc = model.accessors[prim.indices];
        const tg3_buffer_view &idxBV  = model.buffer_views[idxAcc.buffer_view];
        const uint32_t *indexData = reinterpret_cast<const uint32_t *>(
            model.buffers[idxBV.buffer].data.data + idxBV.byte_offset + idxAcc.byte_offset);
        for (uint64_t i = 0; i < idxAcc.count; i++)
          meshData->indices.push_back(indexData[i]);
      }
    }
  }
  tg3_model_free(&model);
  createVertexBuffer();
  createIndexBuffer();

  #if defined(MAGMA_WITH_EDITOR)
    sourcePath = filepath;
  #endif

  return true;
}

void Mesh::createVertexBuffer() {
  assert(meshData != nullptr && "Cannot create vertex buffer before loading mesh data!");

  uint32_t vertexCount = static_cast<uint32_t>(meshData->vertices.size());
  uint32_t vertexSize  = sizeof(meshData->vertices[0]);

  Buffer stagingBuffer(vertexSize, vertexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)meshData->vertices.data());

  vertexBuffer = std::make_unique<Buffer>(vertexSize, vertexCount,
                                         VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  Device::get().copyBuffer(stagingBuffer.getBuffer(), vertexBuffer->getBuffer(), vertexSize * vertexCount);
}

void Mesh::createIndexBuffer() {
  assert(meshData != nullptr && "Cannot create index buffer before loading mesh data!");

  uint32_t indexCount = static_cast<uint32_t>(meshData->indices.size());
  hasIndexBuffer = indexCount > 0;
  if (!hasIndexBuffer) return;

  uint32_t indexSize = sizeof(meshData->indices[0]);

  Buffer stagingBuffer(indexSize, indexCount,
                       VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                       VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
  stagingBuffer.map();
  stagingBuffer.writeToBuffer((void *)meshData->indices.data());

  indexBuffer = std::make_unique<Buffer>(indexSize, indexCount,
                                        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
                                        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
  Device::get().copyBuffer(stagingBuffer.getBuffer(), indexBuffer->getBuffer(), indexSize * indexCount);
}

#if defined(MAGMA_WITH_EDITOR)
void Mesh::onInspector() {
  if (meshData) {
    ImGui::Text("Vertices: %zu", meshData->vertices.size());
    ImGui::Text("Indices: %zu", meshData->indices.size());
  }

  if (!assetsScanned) {
    scanAssetsOnce();
    assetsScanned = true;
  }

  if (pathBuffer[0] == 0 && !sourcePath.empty())
    snprintf(pathBuffer, sizeof(pathBuffer), "%s", sourcePath.c_str());

  ImGui::Spacing();
  ImGui::TextDisabled("Asset: ");

  bool pressedEnter = ImGui::InputText("##mesh_path", pathBuffer, IM_ARRAYSIZE(pathBuffer),
      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

  if (ImGui::IsItemHovered() && Window::hasDroppedText) {
    std::string dropped = Window::getDroppedText();
    snprintf(pathBuffer, sizeof(pathBuffer), "%s", dropped.c_str());
    if (std::find(assets.begin(), assets.end(), dropped) != assets.end()) {
      sourcePath = dropped;
      SceneManager::activeScene->defer(SceneAction::loadMesh(this));
    }
    Window::resetHasDropped();
  }

  if (pressedEnter) {
    std::string typed = pathBuffer;
    if (std::find(assets.begin(), assets.end(), typed) != assets.end()) {
      sourcePath = typed;
      SceneManager::activeScene->defer(SceneAction::loadMesh(this));
    }
  }

  if (!sourcePath.empty()) {
    ImGui::Spacing();
    ImGui::TextDisabled("Current Source: %s", sourcePath.c_str());
  }
}

void Mesh::scanAssetsOnce() {
  const fs::path assetDir = "assets/";
  try {
    if (fs::exists(assetDir) && fs::is_directory(assetDir)) {
      for (auto it = fs::recursive_directory_iterator(assetDir);
           it != fs::recursive_directory_iterator(); it++) {
        if (it->is_regular_file() && string_utils::hasAllowedExt(it->path()))
          assets.push_back(it->path().string());
      }
    }
  } catch (const fs::filesystem_error &e) {
    std::println("Filesystem error: {}", e.what());
  }
  std::sort(assets.begin(), assets.end());
  assets.erase(std::unique(assets.begin(), assets.end()), assets.end());
}
#endif

} // namespace Magma
