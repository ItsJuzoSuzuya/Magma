#pragma once
#include "component.hpp"
#include "core/buffer.hpp"
#include <memory>
#include <string>
#include <vector>

namespace Magma {

class Device;
class MeshData;

class Mesh : public Component {
public:
  Mesh() : Component() {}
  ~Mesh();

  bool load();
  bool load(const std::string &filepath);

  void onUpdate() override {}
  void collectProxy(RenderProxy &proxy) override;

  #if defined(MAGMA_WITH_EDITOR)
    void onInspector() override;
    const char *inspectorName() const override { return "Mesh Renderer"; }
    const float inspectorHeight() const override { return 150.0f; }
  #endif

private:
  MeshData *meshData = nullptr;

  std::unique_ptr<Buffer> vertexBuffer;
  std::unique_ptr<Buffer> indexBuffer;
  bool hasIndexBuffer = false;
  void createVertexBuffer();
  void createIndexBuffer();

  #if defined(MAGMA_WITH_EDITOR)
    std::string sourcePath;
    char pathBuffer[256] = "";
    int popupSelection = -1;
    std::vector<std::string> filteredAssets;

    inline static std::vector<std::string> assets;
    inline static bool assetsScanned = false;
    static void scanAssetsOnce();
  #endif
};

} // namespace Magma
