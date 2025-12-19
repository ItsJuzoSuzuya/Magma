#pragma once
#include "../../core/frame_info.hpp"
#include "../../core/renderer.hpp"
#include "../components/camera.hpp"
#include "render_context.hpp"

#if defined(MAGMA_WITH_EDITOR)
#include "imgui.h"
#include "offscreen_target.hpp"
#include <cstdint>
#else
#include "swapchain_target.hpp"
#endif

namespace Magma {

class RenderTargetInfo;

enum class RendererMode { Game, Editor };

class OffscreenRenderer : public Renderer {
public:
  #if defined(MAGMA_WITH_EDITOR)
    OffscreenRenderer(RenderTargetInfo &info, RenderContext *renderContext,
                    RendererMode mode = RendererMode::Game);

    OffscreenTarget &target() { return *renderTarget; }
    void resize(VkExtent2D newExtent);

    ImVec2 getSceneSize() const;
    ImTextureID getSceneTexture() const {
      return textures[FrameInfo::frameIndex]; }
    void createSceneTextures();

    // Picking API (deferred to safe point)
    void requestPick(uint32_t x, uint32_t y);
    GameObject *pollPickResult();
  #else 
    OffscreenRenderer(SwapChain &swapChain, RenderContext *renderContext, 
                    RendererMode mode = RendererMode::Game);

    SwapchainTarget &target() { return *renderTarget; }
    void resize(VkExtent2D newExtent, VkSwapchainKHR swapChain);
  #endif

  ~OffscreenRenderer();

  void setActiveCamera(Camera *camera) { activeCamera = camera; }
  Camera *getActiveCamera() override { return activeCamera; }

  void begin() override;
  void record() override;
  void end() override;

  void uploadCameraUBO(const CameraUBO &ubo) override;
  void submitPointLight(const PointLightData &lightData) override;

private:
  RenderContext *renderContext = nullptr;
  uint32_t rendererId = 0;
  RendererMode mode;

  Camera *activeCamera = nullptr;

  std::vector<VkImageLayout> sceneColorLayouts; // main color image layout

  #if defined(MAGMA_WITH_EDITOR)
    std::unique_ptr<OffscreenTarget> renderTarget = nullptr;

    std::vector<ImTextureID> textures;
    std::vector<VkImageLayout> idImageLayouts;

    // Deferred Picking
    struct PendingPick {
      bool hasRequest = false;
      uint32_t x = 0, y = 0;
      GameObject *result = nullptr;
    } pendingPick;

    GameObject *pickAtPixel(uint32_t x, uint32_t y);
    void servicePendingPick(); // executes after offscreen rendering
  #else
    std::unique_ptr<SwapchainTarget> renderTarget;
  #endif

};

} // namespace Magma
