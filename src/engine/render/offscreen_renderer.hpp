#pragma once
#include "../../core/frame_info.hpp"
#include "../../core/renderer.hpp"
#include "../../core/buffer.hpp"
#include "offscreen_target.hpp"
#include "swapchain_target.hpp"

#if defined(MAGMA_WITH_EDITOR)
#include "imgui.h"
#endif

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
  #if defined(MAGMA_WITH_EDITOR)
  ImVec2 getSceneSize() const;
  ImTextureID getSceneTexture() const {
    return textures[FrameInfo::frameIndex];
  }
  #endif
  Buffer *getCameraBuffer() override { return cameraBuffers[FrameInfo::frameIndex].get(); }
  #if defined(MAGMA_WITH_EDITOR)
  OffscreenTarget &target() { return *renderTarget; }
  #else
  SwapchainTarget &target() { return *renderTarget; }
  #endif
  VkRenderPass getRenderPass() { return renderTarget->getRenderPass(); }

  // Textures
  #if defined(MAGMA_WITH_EDITOR)
  void createOffscreenTextures();
  #endif

  // Rendering
  void begin() override;
  void record() override;
  void end() override;

  // Resize
  void resize(VkExtent2D newExtent);

  GameObject *pickAtPixel(uint32_t x, uint32_t y);

private:
  #if defined(MAGMA_WITH_EDITOR)
  // Textures for ImGui
  std::vector<ImTextureID> textures;
  #endif

  // RenderTarget for picking Objects in the scene

  #if defined(MAGMA_WITH_EDITOR)
  std::unique_ptr<OffscreenTarget> renderTarget;
  #else
  std::unique_ptr<SwapchainTarget> renderTarget;
  #endif

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
