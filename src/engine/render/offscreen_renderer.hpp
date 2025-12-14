#pragma once
#include "../../core/frame_info.hpp"
#include "../../core/renderer.hpp"
#include "../components/camera.hpp"
#include "render_context.hpp"
#include <atomic>

#if defined(MAGMA_WITH_EDITOR)
#include "imgui.h"
#include "offscreen_target.hpp"
#include <cstdint>
#else
#include "swapchain_target.hpp"
#endif

namespace Magma {

class RenderTargetInfo;

class OffscreenRenderer : public Renderer {
public:
#if defined(MAGMA_WITH_EDITOR)
  OffscreenRenderer(RenderTargetInfo &info, RenderContext *renderContext,
                    bool isEditorRenderer = false);
#else
  OffscreenRenderer(SwapChain &swapChain, RenderContext *renderContext);
#endif

  ~OffscreenRenderer();

  VkImage &getSceneImage() const;

  void setActiveCamera(Camera *camera) { activeCamera = camera; }
  Camera *getActiveCamera() override { return activeCamera; }

#if defined(MAGMA_WITH_EDITOR)
  OffscreenTarget &target() { return *renderTarget; }
#else
  SwapchainTarget &target() { return *renderTarget; }
#endif

// Textures
#if defined(MAGMA_WITH_EDITOR)
  ImVec2 getSceneSize() const;
  ImTextureID getSceneTexture() const {
    return textures[FrameInfo::frameIndex];
  }
  void createOffscreenTextures();

  // Picking API (deferred to safe point)
  void requestPick(uint32_t x, uint32_t y);
  GameObject *pollPickResult();
#endif

  // Rendering
  void begin() override;
  void record() override;
  void end() override;

// Resize
#if defined(MAGMA_WITH_EDITOR)
  void resize(VkExtent2D newExtent);
  GameObject *pickAtPixel(uint32_t x, uint32_t y);
#else
  void resize(VkExtent2D newExtent, VkSwapchainKHR swapChain);
#endif

  void uploadCameraUBO(const CameraUBO &ubo) override;
  void submitPointLight(const PointLightData &lightData) override;

private:
#if defined(MAGMA_WITH_EDITOR)
  // Textures for ImGui
  std::vector<ImTextureID> textures;

  bool isEditorRenderer = false;
#endif

  // RenderTarget for picking Objects in the scene
#if defined(MAGMA_WITH_EDITOR)
  std::unique_ptr<OffscreenTarget> renderTarget;
#else
  std::unique_ptr<SwapchainTarget> renderTarget;
#endif

  // Render context
  RenderContext *renderContext = nullptr;

  Camera *activeCamera = nullptr;

  inline static std::atomic<uint32_t> nextRendererId{0};
  uint32_t rendererId = 0;

  // Image layout tracking (per-frame images)
  std::vector<VkImageLayout> sceneColorLayouts; // main color image layout
#if defined(MAGMA_WITH_EDITOR)
  std::vector<VkImageLayout> idColorLayouts; // ID image layout

  // Deferred pick request state
  struct PendingPick {
    bool hasRequest = false;
    uint32_t x = 0, y = 0;
    GameObject *result = nullptr;
  } pendingPick;

  void servicePendingPick(); // executes after offscreen pass ends
#endif
};

} // namespace Magma
