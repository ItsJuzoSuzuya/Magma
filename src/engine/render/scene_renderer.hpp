#pragma once
#include "core/pipeline.hpp"
#include "core/render_target.hpp"
#include "core/renderer.hpp"
#include "engine/components/camera.hpp"
#include "engine/editor_camera.hpp"
#include "engine/render/features/object_picker.hpp"
#include "engine/render/swapchain_target.hpp"
#include "imgui.h"
#include "render_context.hpp"
#include <cstdint>
#include <memory>
#include <vector>
#include <vulkan/vulkan_core.h>

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

  uint32_t rendererId = 0;

  void setCameraSource(CameraSource source) {
    cameraSource = source; 
  }
  static EditorCamera* getEditorCamera(){
    return editorCamera.get(); 
  }

  void createSceneTextures();
  ImVec2 getSceneSize() const;
  ImTextureID getSceneTexture(size_t index) const;

  VkPipelineLayout getPipelineLayout() const override {
    return pipelineLayout; }
  
  ObjectPicker &getObjectPicker() { return *objectPicker; }

  void onResize(const VkExtent2D newExtent) override;
  void onRender() override;

  bool isSwapChainDependent() const override { return isSwapChainDependentFlag; }
  SwapChain* getSwapChain() const override;

  void uploadCameraUBO(const CameraUBO &ubo);
  void submitPointLight(const PointLightData &lightData);


private:
  std::unique_ptr<Pipeline> pipeline = nullptr;
  PipelineShaderInfo shaderInfo;
  void createPipeline() override;

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  void createPipelineLayout(
      const std::vector<VkDescriptorSetLayout> &layouts) override;

  void begin() override;
  void record() override;
  void end() override;

  std::unique_ptr<IRenderTarget> renderTarget = nullptr;
  std::unique_ptr<RenderContext> renderContext = nullptr;
  bool isSwapChainDependentFlag = false;

  std::vector<ImTextureID> sceneTextures = {};

  CameraSource cameraSource = CameraSource::Editor;
  inline static std::unique_ptr<EditorCamera> editorCamera = std::make_unique<EditorCamera>();

  std::unique_ptr<ObjectPicker> objectPicker;

};

} // namespace Magma
