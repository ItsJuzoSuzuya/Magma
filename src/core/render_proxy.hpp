#pragma once
#include "core/mesh_data.hpp"
#include <glm/ext/matrix_float4x4.hpp>
#include <optional>
#include <vulkan/vulkan_core.h>

namespace Magma {

struct MeshProxy {
    MeshData *meshData;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer  = VK_NULL_HANDLE;
    uint32_t indexCount   = 0;
    uint32_t vertexCount  = 0;
    bool     hasIndexBuffer = false;
};

struct TransformProxy {
    glm::mat4 modelMatrix{1.f};
    uint32_t  objectId = 0;
};

struct PointLightProxy {
  glm::vec4 position;
  glm::vec4 color;
};

struct CameraProxy {
    glm::mat4 projView{1.f};
};

// A complete renderable object — built by GameObject each frame
struct RenderProxy {
    std::optional<MeshProxy>       mesh;
    std::optional<TransformProxy>  transform;
    std::optional<PointLightProxy> pointLight;
    std::optional<CameraProxy>     camera;
};

} // namespace Magma
