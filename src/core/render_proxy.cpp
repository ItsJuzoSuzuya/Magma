module;
#include <glm/ext/matrix_float4x4.hpp>
#include <optional>
#include <vulkan/vulkan_core.h>
#include <memory>

export module core:render_proxy;
import :mesh_data;

namespace Magma {

export struct MeshProxy {
    MeshData *meshData;
    VkBuffer vertexBuffer = VK_NULL_HANDLE;
    VkBuffer indexBuffer  = VK_NULL_HANDLE;
    uint32_t indexCount   = 0;
    uint32_t vertexCount  = 0;
    bool     hasIndexBuffer = false;
};

export struct TransformProxy {
    glm::mat4 modelMatrix{1.f};
    uint32_t  objectId = 0;
};

export struct PointLightProxy {
  glm::vec4 position;
  glm::vec4 color;
};

export struct CameraProxy {
    glm::mat4 projView{1.f};
};

// A complete renderable object - built by GameObject each frame
export struct RenderProxy {
    std::optional<MeshProxy>       mesh;
    std::optional<TransformProxy>  transform;
    std::optional<PointLightProxy> pointLight;
    std::optional<CameraProxy>     camera;
};

}
