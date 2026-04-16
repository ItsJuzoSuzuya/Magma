#pragma once
#include "core/frame_info.hpp"
#include "core/push_constant_data.hpp"
#include "core/render_proxy.hpp"
#include "core/renderer.hpp"
#include <cassert>
#include <print>
#include <vulkan/vulkan_core.h>

namespace Magma {

class RenderCallback {
public:
  static void renderMesh(const MeshProxy &mesh) {
    if (!mesh.meshData) return;

    VkBuffer vertexBuffers[] = {mesh.vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(FrameInfo::commandBuffer, 0, 1, vertexBuffers, offsets);

    if (mesh.hasIndexBuffer) {
      assert(mesh.indexBuffer != VK_NULL_HANDLE &&
             "Index buffer must be valid before rendering indexed mesh!");
      vkCmdBindIndexBuffer(FrameInfo::commandBuffer, mesh.indexBuffer, 0,
                           VK_INDEX_TYPE_UINT32);
    }

    if (mesh.hasIndexBuffer)
      vkCmdDrawIndexed(FrameInfo::commandBuffer, mesh.indexCount, 1, 0, 0, 0);
    else
      vkCmdDraw(FrameInfo::commandBuffer, mesh.vertexCount, 1, 0, 0);
  }

  static void renderTransform(IRenderer &renderer, const TransformProxy &transform) {
    PushConstantData push{};
    push.modelMatrix = transform.modelMatrix;
    push.objectId = transform.objectId;

    vkCmdPushConstants(FrameInfo::commandBuffer, renderer.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData),
                       &push);
  }
};

} // namespace Magma
