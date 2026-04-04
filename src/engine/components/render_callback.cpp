module;
#include <cassert>
#include <vulkan/vulkan_core.h>

module components:render_callback;
import core:FrameInfo;

export class RenderCallback {
  static void renderMesh(SceneRenderer &renderer, MeshProxy &mesh) {
    if (!mesh.meshData)
      return;

    VkBuffer vertexBuffers[] = {mesh.vertexBuffer->getBuffer()};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(FrameInfo::commandBuffer, 0, 1, mesh.vertexBuffers,
                           offsets);

    if (mesh.hasIndexBuffer){
      assert(mesh.indexBuffer != nullptr &&
             "Index buffer must be created before rendering indexed mesh!");
      vkCmdBindIndexBuffer(FrameInfo::commandBuffer, mesh.indexBuffer->getBuffer(), 0,
                           VK_INDEX_TYPE_UINT32);
    }
  }

  // Lifecycle
  static void renderTransform(Renderer &renderer, TransformProxy &transform) {
    PushConstantData push{};
    push.modelMatrix = transform.mat4();

    if(owner && !owner->getComponent<Camera>())
      push.objectId = transform.objectId;
    else 
      push.objectId = 0;

    vkCmdPushConstants(FrameInfo::commandBuffer, renderer.getPipelineLayout(),
                       VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(PushConstantData),
                       &push);
  }

  static void renderPointLight(SceneRenderer &renderer, PointLightProxy &pointLight) {
    // Keep light position in sync with the owner's Transform
    if (owner) {
      if (auto *t = owner->getComponent<Transform>())
        lightData.position = glm::vec4(t->position, 1.0f);
    }
    renderer.submitPointLight(lightData);
  }

  static void renderCamera(SceneRenderer &renderer, CameraProxy camera) {
    if (ownerTransform)
      setView(ownerTransform->position, ownerTransform->rotation);

    CameraUBO ubo{};
    ubo.projectionView = camera.projectionView;
    

    renderer.uploadCameraUBO(ubo);
  }
}

}
