module;
#include <cassert>
#include <vulkan/vulkan_core.h>

export module render:render_callback;
import core;
import components;

namespace Magma {

export class RenderCallback {
public:
  static void renderMesh(IRenderer &renderer, const MeshProxy &mesh) {
    if (!mesh.meshData)
      return;

    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(FrameInfo::commandBuffer, 0, 1, vertexBuffers, offsets);

    if (mesh.hasIndexBuffer) {
      assert(mesh.indexBuffer != VK_NULL_HANDLE &&
             "Index buffer must be valid before rendering indexed mesh!");
      vkCmdBindIndexBuffer(FrameInfo::commandBuffer, mesh.indexBuffer, 0,
                           VK_INDEX_TYPE_UINT32);
    }
  }

  static void renderTransform(IRenderer &renderer, const TransformProxy &transform) {
    PushConstantData push{};
    push.modelMatrix = transform.modelMatrix;
    push.objectId = transform.objectId;

    vkCmdPushConstants(FrameInfo::commandBuffer, renderer.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData),
                       &push);
  }

  static void renderPointLight(IRenderer &renderer, const PointLightProxy &pointLight) {
    (void)renderer;
    (void)pointLight;
    // Point light data is uploaded via SceneRenderer::submitPointLight
  }

  static void renderCamera(IRenderer &renderer, const CameraProxy &camera) {
    (void)renderer;
    (void)camera;
    // Camera data is uploaded via SceneRenderer::uploadCameraUBO
  }
};

} // namespace Magma
