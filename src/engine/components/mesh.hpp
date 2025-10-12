#pragma once
#include "../core/buffer.hpp"
#include "component.hpp"
#include <memory>
#include <string>

namespace Magma {

class Device;
class MeshData;

class Mesh : public Component {
public:
  Mesh(Device &device);
  ~Mesh();

  // Data loading
  bool load(const std::string &filepath);

  // Lifecycle
  void onRender(Renderer &renderer) override;
  void draw();
  void onAwake() override {};
  void onUpdate() override {};

  // Inspector
  void onInspector() override {};

private:
  Device &device;
  MeshData *meshData;

  // Buffers
  std::unique_ptr<Buffer> vertexBuffer;
  std::unique_ptr<Buffer> indexBuffer;
  bool hasIndexBuffer = false;
  void createVertexBuffer();
  void createIndexBuffer();
};

} // namespace Magma
