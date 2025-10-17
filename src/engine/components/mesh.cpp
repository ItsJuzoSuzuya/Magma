#include "mesh.hpp"
#include "../../core/window.hpp"
#include "../core/device.hpp"
#include "../core/frame_info.hpp"
#include "../core/mesh_data.hpp"
#include "../scene.hpp"
#include "../scene_action.hpp"
#include "component.hpp"
#include "imgui.h"
#include <X11/X.h>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <iterator>
#include <memory>
#include <print>

#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../external/tiny_gltf.h"

using namespace std;
namespace fs = std::filesystem;

namespace Magma {

namespace string_utils {
inline string toLower(string str) {
  transform(str.begin(), str.end(), str.begin(),
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

Mesh::Mesh(GameObject *owner, Device &device)
    : Component(owner), device{device} {
  println("Mesh created");
}

Mesh::~Mesh() {
  if (meshData) {
    delete meshData;
    meshData = nullptr;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
  }
}

// --- Public --- //
bool Mesh::load() {
  if (sourcePath.empty()) {
    println("No source path set for mesh.");
    return false;
  }
  return load(sourcePath);
}

// --- Data ---
bool Mesh::load(const string &filepath) {
  if (meshData) {
    delete meshData;
    meshData = nullptr;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
  }

  tinygltf::Model gltfModel;
  tinygltf::TinyGLTF loader;
  string err;
  string warn;

  bool result = loader.LoadASCIIFromFile(&gltfModel, &err, &warn, filepath);

  if (!warn.empty())
    println("Warning: {}", warn);

  if (!err.empty()) {
    println("Error: {}", err);
    return false;
  }

  if (!result) {
    println("Failed to load glTF model: {}", filepath);
    return false;
  } else {
    println("Loading glTF model: {}", filepath);
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

  sourcePath = filepath;

  return true;
}

// --- Lifecycle ---
void Mesh::onRender(Renderer &renderer) {
  VkBuffer vertexBuffers[] = {vertexBuffer->getBuffer()};
  VkDeviceSize offsets[] = {0};
  vkCmdBindVertexBuffers(FrameInfo::commandBuffer, 0, 1, vertexBuffers,
                         offsets);

  if (hasIndexBuffer)
    vkCmdBindIndexBuffer(FrameInfo::commandBuffer, indexBuffer->getBuffer(), 0,
                         VK_INDEX_TYPE_UINT32);
}

void Mesh::draw() {
  if (hasIndexBuffer)
    vkCmdDrawIndexed(FrameInfo::commandBuffer,
                     static_cast<uint32_t>(meshData->indices.size()), 1, 0, 0,
                     0);
  else
    vkCmdDraw(FrameInfo::commandBuffer,
              static_cast<uint32_t>(meshData->vertices.size()), 1, 0, 0);
}

// --- Inspector ---
void Mesh::onInspector() {
  ImGui::TextUnformatted("Mesh");
  ImGui::Separator();

  if (meshData) {
    ImGui::Text("Vertices: %zu", meshData->vertices.size());
    ImGui::Text("Indices: %zu", meshData->indices.size());
  }

  if (!assetsScanned) {
    scanAssetsOnce();
    assetsScanned = true;
  }

  if (pathBuffer[0] == 0) {
    if (!sourcePath.empty()) {
      snprintf(pathBuffer, size(sourcePath), "%s", sourcePath.c_str());
    } else {
      snprintf(pathBuffer, size(sourcePath), "assets/");
    }
  }

  ImGui::Spacing();
  ImGui::TextDisabled("Asset: ");

  bool pressedEnter = ImGui::InputText(
      "##mesh_path", pathBuffer, IM_ARRAYSIZE(pathBuffer),
      ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);

  if (ImGui::IsItemHovered() && Window::hasDroppedText) {
    string dropped = Window::getDroppedText();
    snprintf(pathBuffer, sizeof(pathBuffer), "%s", dropped.c_str());

    if (find_if(assets.begin(), assets.end(), [&dropped](const string &asset) {
          return asset == dropped;
        }) != assets.end()) {
      sourcePath = dropped;
      Scene::current()->defer(SceneAction::loadMesh(owner));
    }
    Window::resetDrop();
  }

  /*
  // Accept drag-and-drop onto the input (payload must be a NUL-terminated
  path) if (ImGui::BeginDragDropTarget()) { if (const ImGuiPayload *payload =
            ImGui::AcceptDragDropPayload("FILE_PATH")) {
      const char *path = reinterpret_cast<const char *>(payload->Data);
      if (path && *path) {
        std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", path);
        if (load(path))
          sourcePath = path;
      }
    }
    ImGui::EndDragDropTarget();
  }


  bool inputActive = ImGui::IsItemFocused();
  if (inputActive) {
    println("Input active");
    string query = string_utils::toLower(pathBuffer);
    println("Query: {}", query);
    filteredAssets.clear();

    if (!query.empty()) {
      for (const auto &asset : assets) {
        println("Checking asset: {}", asset);
        if (string_utils::toLower(asset).find(query) != string::npos)
          filteredAssets.push_back(asset);
        if (filteredAssets.size() >= 20)
          break;
      }
    }

    if (popupSelection >= (int)filteredAssets.size())
      popupSelection = 0;
    if (popupSelection < 0 && !filteredAssets.empty())
      popupSelection = (int)filteredAssets.size() - 1;
  }

    ImVec2 pos = ImGui::GetItemRectMin();
    pos.y = ImGui::GetItemRectMax().y;
    ImGui::SetNextWindowPos(pos);
    ImGui::SetNextWindowSizeConstraints(
        ImVec2(ImGui::GetItemRectSize().x, 0.f),
        ImVec2(ImGui::GetItemRectSize().x, 240.f));

    ImGui::OpenPopup("##mesh_path_popup");
  } else {
    popupSelection = -1;
  }
  println("Input active: {}, popupSelection: {}", inputActive,
  popupSelection);

  bool pickedFromPopup = false;
  string pickedPath;

  if (ImGui::BeginPopup("##mesh_path_popup")) {
    // Keyboard navigation
    if (ImGui::IsKeyPressed(ImGuiKey_DownArrow))
      popupSelection++;
    if (ImGui::IsKeyPressed(ImGuiKey_UpArrow))
      popupSelection--;

    int idx = 0;
    for (const auto &item : filteredAssets) {
      bool selected = (idx == popupSelection);
      if (ImGui::Selectable(item.c_str(), selected)) {
        pickedFromPopup = true;
        pickedPath = item;
      }
      if (selected)
        ImGui::SetItemDefaultFocus();
      idx++;
    }

    ImGui::EndPopup();
  }

  if (pickedFromPopup) {
    std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", pickedPath.c_str());
    if (load(pickedPath))
      sourcePath = pickedPath;

    ImGui::CloseCurrentPopup();
  }
  */
  if (pressedEnter) {
    std::string typed = pathBuffer;
    /*// Prefer highlighted suggestion if popup is open and we have one
    if (ImGui::IsPopupOpen("##mesh_path_popup") && !filteredAssets.empty() &&
        popupSelection >= 0 && popupSelection < (int)filteredAssets.size()) {
      typed = filteredAssets[popupSelection];
      std::snprintf(pathBuffer, sizeof(pathBuffer), "%s", typed.c_str());
    }
    */

    if (find_if(assets.begin(), assets.end(), [&typed](const string &asset) {
          return asset == typed;
        }) != assets.end()) {
      sourcePath = typed;
      Scene::current()->defer(SceneAction::loadMesh(owner));
    }
  }

  // Display current source
  if (!sourcePath.empty()) {
    ImGui::Spacing();
    ImGui::TextDisabled("Loaded:");
    ImGui::TextWrapped("%s", sourcePath.c_str());
  }
}

// --- Private --- //
// --- Buffers ---
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

// --- Assets ---
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
    println("Filesystem error: {}", e.what());
  }

  std::sort(assets.begin(), assets.end());
  assets.erase(std::unique(assets.begin(), assets.end()), assets.end());
}

} // namespace Magma
