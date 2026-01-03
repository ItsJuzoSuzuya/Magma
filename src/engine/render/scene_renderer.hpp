#pragma once
#include "core/frame_info.hpp"
#include "core/pipeline.hpp"
#include "core/render_target.hpp"
#include "core/renderer.hpp"
#include "engine/components/camera.hpp"
#include "engine/render/features/object_picker.hpp"
#include "imgui.h"
#include "render_context.hpp"
#include <algorithm>
#include <cstdint>
#include <memory>

namespace Magma {

class SceneRenderer : public IRenderer {
public:
  SceneRenderer(std::unique_ptr<IRenderTarget> target, PipelineShaderInfo &shaderInfo);
  ~SceneRenderer();
  void destroy() override;

  ImVec2 getSceneSize() const;
  VkPipelineLayout getPipelineLayout() const override {
    return pipelineLayout; }

  void onResize(const VkExtent2D newExtent) override;
  void onRender() override;

  bool isSwapChainDependent() const override { return isSwapChainDependentFlag; }

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
  bool isSwapChainDependentFlag = true;

  Camera *activeCamera = nullptr;

  ObjectPicker objectPicker;

};

} // namespace Magma
