#pragma once
#include "core/buffer.hpp"
#include "core/descriptors.hpp"
#include "core/pipeline.hpp"
#include "core/render_proxy.hpp"
#include "core/render_target.hpp"
#include "core/renderer.hpp"
#include "core/swapchain.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/point_light.hpp"
#include "engine/render/features/render_feature.hpp"
#include "engine/render/render_context.hpp"
#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
#endif

namespace Magma {

enum class CameraSource {
  Editor,
  Scene
};

class SceneRenderer : public IRenderer {
public:
  SceneRenderer(std::unique_ptr<IRenderTarget> target, PipelineShaderInfo &shaderInfo);
  ~SceneRenderer();

  void destroy() override;

  void initPipeline(RenderContext *rc){
    renderContext = rc;

    std::vector<VkDescriptorSetLayout> layouts = {
        cameraLayout->getDescriptorSetLayout(),
        renderContext->getLayout(LayoutKey::PointLight)};

    createPipelineLayout(layouts);
    createPipeline();
  }

  template<typename T> 
  T &getFeature() {
    for (auto &feature : renderFeatures) {
      if (auto casted = dynamic_cast<T*>(feature.get()))
        return *casted;
    }
    throw std::runtime_error("RenderFeature of requested type not found.");
  }
  void addRenderFeature(std::unique_ptr<RenderFeature> feature);

  CameraSource cameraSource = CameraSource::Editor;
  static void setEditorCameraProxy(const RenderProxy &proxy) {
    editorCameraProxy = proxy;
  }

  #if defined(MAGMA_WITH_EDITOR)
    void createSceneTextures();
    ImVec2 getSceneSize() const;
    ImTextureID getSceneTexture(size_t index) const;
  #endif

  VkPipelineLayout getPipelineLayout() const override {
    return pipelineLayout; }

  void onResize(const VkExtent2D newExtent) override;
  void onRender() override;

  bool isSwapChainDependent() const override { return isSwapChainDependentFlag; }
  SwapChain* getSwapChain() const override;

  void uploadCameraUBO(const CameraUBO &ubo);

private:
  std::unique_ptr<Pipeline> pipeline = nullptr;
  PipelineShaderInfo shaderInfo;
  void createPipeline() override;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  void createPipelineLayout(
      const std::vector<VkDescriptorSetLayout> &layouts) override;

  std::unique_ptr<DescriptorSetLayout> cameraLayout;
  std::unique_ptr<DescriptorPool> cameraPool;
  std::array<std::unique_ptr<Buffer>, SwapChain::MAX_FRAMES_IN_FLIGHT> cameraUBOs;
  std::array<VkDescriptorSet, SwapChain::MAX_FRAMES_IN_FLIGHT> cameraDescriptorSets{};

  void begin() override;
  void record() override;
  void end() override;

  std::vector<RenderProxy> getSceneProxies();
  void submitProxy(const RenderProxy &proxy);

  RenderContext *renderContext;
  std::unique_ptr<IRenderTarget> renderTarget = nullptr;
  std::vector<std::unique_ptr<RenderFeature>> renderFeatures = {};
  bool isSwapChainDependentFlag = false;

#if defined(MAGMA_WITH_EDITOR)
  std::vector<ImTextureID> sceneTextures = {};
#endif

  inline static RenderProxy editorCameraProxy = {};
};

} // namespace Magma
