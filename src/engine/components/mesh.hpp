#pragma once
#include "../core/buffer.hpp"
#include "component.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Magma {

class Device;
class MeshData;

/**
 * Mesh component that holds mesh data and handles rendering.
 */
class Mesh : public Component {
public:
  Mesh(GameObject *owner);
  ~Mesh();

  // --- Data ---
  /**
   * Load mesh from the current source path.
   * @return True if the mesh was loaded successfully, false otherwise.
   * @note Currently supports only .gltf files.
   */
  bool load();

  /**
   * Load mesh from file.
   * @param filepath Path to the mesh file.
   * @return True if the mesh was loaded successfully, false otherwise.
   * @note Currently supports only .gltf files.
   */
  bool load(const std::string &filepath);

  // --- Lifecycle ---
  void onAwake() override {};
  void onUpdate() override {};
  void onRender(SceneRenderer &renderer) override;
  void draw();

  #if defined(MAGMA_WITH_EDITOR)
  // Inspector
  void onInspector() override;
  const char *inspectorName() const override { return "Mesh Renderer"; }
  const float inspectorHeight() const override { return 150.0f; }
  #endif

private:
  MeshData *meshData = nullptr;

  // --- Buffers ---
  std::unique_ptr<Buffer> vertexBuffer;
  std::unique_ptr<Buffer> indexBuffer;
  bool hasIndexBuffer = false;
  void createVertexBuffer();
  void createIndexBuffer();

  #if defined(MAGMA_WITH_EDITOR)
  // --- Inspector ---
  std::string sourcePath;
  char pathBuffer[256] = "";
  int popupSelection = -1;
  std::vector<std::string> filteredAssets;

  // --- Assets Cache ---

  inline static std::vector<std::string> assets;
  inline static bool assetsScanned = false;
  static void scanAssetsOnce();
  #endif
};

} // namespace Magma
