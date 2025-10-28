#pragma once
#include "../../core/frame_info.hpp"
#include "../../core/renderer.hpp"
#include "../../core/buffer.hpp"
#include "imgui.h"
#include "offscreen_target.hpp"
#include <cstdint>

namespace Magma {

class RenderTargetInfo;

class OffscreenRenderer : public Renderer {
public:
  // Constructor
  OffscreenRenderer(RenderTargetInfo &info);
  // Destructor
  ~OffscreenRenderer();

  // Getters
  VkImage &getSceneImage() const;
  ImVec2 getSceneSize() const;
  ImTextureID getSceneTexture() const {
    return textures[FrameInfo::frameIndex];
  }
  Buffer *getCameraBuffer(uint32_t i) override { return cameraBuffers[i].get(); }
  OffscreenTarget &target() { return *renderTarget; }
  VkRenderPass getRenderPass() { return renderTarget->getRenderPass(); }

  // Textures
  void createOffscreenTextures();

  // Rendering
  void begin() override;
  void record() override;
  void end() override;

  // Resize
  void resize(VkExtent2D newExtent);

private:
  // Textures for ImGui
  std::vector<ImTextureID> textures;

  // RenderTarget for picking Objects in the scene
  std::unique_ptr<OffscreenTarget> renderTarget;

  // Descriptors
  std::vector<VkDescriptorSet> descriptorSets;
  void createDescriptorPool() override;
  void createDescriptorSetLayout() override;
  void createDescriptorSets();

  // Camera Buffer
  std::vector<std::unique_ptr<Buffer>> cameraBuffers;
  void createCameraBuffer();
};

} // namespace Magma
