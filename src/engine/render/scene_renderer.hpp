#pragma once
#include "core/pipeline.hpp"
#include "core/render_target.hpp"
#include "core/renderer.hpp"
#include "engine/components/camera.hpp"
#include "engine/render/features/object_picker.hpp"
#include "engine/render/swapchain_target.hpp"
#include "imgui.h"
#include "render_context.hpp"
#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

namespace Magma {

class SceneRenderer : public IRenderer {
public:
  SceneRenderer(std::unique_ptr<IRenderTarget> target, PipelineShaderInfo &shaderInfo);
  ~SceneRenderer();
  void destroy() override;

  ImVec2 getSceneSize() const;
  ImTextureID getSceneTexture() const;

  VkPipelineLayout getPipelineLayout() const override {
    return pipelineLayout; }
  
  ObjectPicker &getObjectPicker() { return *objectPicker; }

  void onResize(const VkExtent2D newExtent) override;
  void onRender() override;

  bool isSwapChainDependent() const override { return isSwapChainDependentFlag; }
  SwapChain* getSwapChain() const override;

  void setActiveCamera(Camera *camera) { activeCamera = camera; }
  Camera *getActiveCamera() const { return activeCamera; }

  void uploadCameraUBO(const CameraUBO &ubo);
  void submitPointLight(const PointLightData &lightData);

  void createSceneTextures();

private:
  void begin() override;
  void record() override;
  void end() override;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  void createPipelineLayout(
      const std::vector<VkDescriptorSetLayout> &layouts) override;

  std::unique_ptr<Pipeline> pipeline = nullptr;
  PipelineShaderInfo shaderInfo;
  void createPipeline() override;

  std::unique_ptr<IRenderTarget> renderTarget = nullptr;
  std::unique_ptr<RenderContext> renderContext = nullptr;
  uint32_t rendererId = 0;
  bool isSwapChainDependentFlag = false;

  std::vector<ImTextureID> sceneTextures = {};

  Camera *activeCamera = nullptr;

  std::unique_ptr<ObjectPicker> objectPicker;

};

} // namespace Magma
