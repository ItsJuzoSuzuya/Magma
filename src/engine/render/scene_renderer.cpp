module;
#include <iterator>
#include <vector>
#include <memory>
#include <vulkan/vulkan_core.h>

export module render:scene_renderer;
import core;
import features;
import :pipeline_shader_info;

namespace Magma {

class Transform;
class EditorCamera;

export enum class CameraSource {
  Editor,
  Scene
};

export class SceneRenderer : public IRenderer {
public:
  SceneRenderer(
    std::unique_ptr<IRenderTarget> target, 
    PipelineShaderInfo &shaderInfo
  ): IRenderer(), shaderInfo{shaderInfo} {
    renderContext = std::make_unique<RenderContext>();
    renderTarget = std::move(target);

    if (auto *swapchainTarget = dynamic_cast<SwapchainTarget*>(renderTarget.get())) 
      isSwapChainDependentFlag = true;

    // Self-register with the context to get our slice index
    rendererId = renderContext->registerRenderer();

    std::vector<VkDescriptorSetLayout> layouts = {
        renderContext->getLayout(LayoutKey::Camera),
        renderContext->getLayout(LayoutKey::PointLight)};

    createPipelineLayout(layouts);
    renderContext->createDescriptorSets(LayoutKey::Camera);
    renderContext->createDescriptorSets(LayoutKey::PointLight);

    createPipeline();
  }

  ~SceneRenderer() { destroy(); }
  void destroy() override {
    Device::waitIdle();

    renderContext.reset();
    pipeline.reset();

    if (pipelineLayout != VK_NULL_HANDLE) {
      vkDestroyPipelineLayout(Device::get().device(), pipelineLayout, nullptr);
      pipelineLayout = VK_NULL_HANDLE;
    }
  }

  uint32_t rendererId = 0;

  template<typename T>
  T &getFeature() {
    for (auto &feature : renderFeatures) {
      if (auto casted = dynamic_cast<T*>(feature.get()))
        return *casted;
    }
    throw std::runtime_error("RenderFeature of requested type not found.");
  }
  void addRenderFeature(std::unique_ptr<RenderFeature> feature){
    renderFeatures.push_back(std::move(feature));
  }

  CameraSource cameraSource = CameraSource::Editor;
  static EditorCamera* getEditorCamera() {
    return editorCamera.get(); }
  }

  #if defined(MAGMA_WITH_EDITOR)
    void createSceneTextures(){
      assert(renderTarget != nullptr && 
             "Cannot create scene textures for null render target!");

      sceneTextures.resize(renderTarget->imageCount());
      for (uint32_t i = 0; i < sceneTextures.size(); ++i) {
        VkSampler sampler = renderTarget->getColorSampler();
        VkImageView view  = renderTarget->getColorImageView(i);

        sceneTextures[i] = (ImTextureID)ImGui_ImplVulkan_AddTexture(
            sampler, view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
      }
    }
  #endif
  ImVec2 getSceneSize() const {
    return ImVec2(static_cast<float>(renderTarget->extent().width),
                  static_cast<float>(renderTarget->extent().height));
  }
  ImTextureID getSceneTexture(size_t index) const {
    assert(index >= 0 &&
           index < renderTarget->imageCount() &&
           "SceneRenderer: Invalid image index in FrameInfo!");

    return sceneTextures.at(index);
  }

  VkPipelineLayout getPipelineLayout() const override {
    return pipelineLayout; }

  void onResize(const VkExtent2D newExtent) override {
    Device::waitIdle();

    for (auto &tex : sceneTextures) 
      ImGui_ImplVulkan_RemoveTexture((VkDescriptorSet)tex);

    renderTarget->onResize(newExtent);
    for (auto &feature : renderFeatures)
      feature->onResize(newExtent);

    createPipeline();

    sceneTextures.clear();
    createSceneTextures();
  }
  void onRender() override {
    begin();
    record();

    if (cameraSource == CameraSource::Editor && editorCamera) {
      auto camProxy = editorCamera->collectProxy();
      if (camProxy.camera)
          uploadCameraUBO({camProxy.camera->projView});
    }

    auto *activeScene = Scene::getActiveScene()
    if (!activeScene) return;

    for (auto *gameObject : activeScene->getGameObjects()) {
      if (!gameObject) continue;
      for (auto& proxy : go->collectProxies())
          submitProxy(proxy);
    }

    end();
  }

  void submitProxy(const RenderProxy& proxy) {
    if (proxy.transform) 
      RendererCallback::renderTransform(this, proxy.transform);

    if (proxy.pointLight) 
      RendererCallback::renderPointLight(this, proxy.pointLight);

    if (proxy.mesh) 
      RenderCallback::renderMesh(this, proxy.mesh);

    if (proxy.camera) 
      RenderCallback::renderCamera(this, proxy.camera);
  }

  bool isSwapChainDependent() const override { return isSwapChainDependentFlag; }
  SwapChain* getSwapChain() const override {
    #if defined(MAGMA_WITH_EDITOR)
      if (auto *swapchainTarget = dynamic_cast<SwapchainTarget*>(renderTarget.get()))
        return swapchainTarget->swapChain();
      else
        return nullptr;
    #else
      return nullptr;
    #endif
  }

  void uploadCameraUBO(const CameraUBO &ubo){
    if (!renderContext)
      return;
    renderContext->updateCameraSlice(FrameInfo::frameIndex, rendererId,
                                     (void *)&ubo, sizeof(ubo));
  }

  void submitPointLight(const PointLightData &lightData){
    if (!renderContext)
      return;

    PointLightSSBO ssbo{};
    ssbo.lightCount = 1;
    ssbo.lights[0] = lightData;

    renderContext->updatePointLightSlice(FrameInfo::frameIndex, rendererId,
                                         (void *)&ssbo, sizeof(ssbo));
  }

private:
  std::unique_ptr<Pipeline> pipeline = nullptr;
  PipelineShaderInfo shaderInfo;
  void createPipeline() override {
    assert(pipelineLayout != nullptr &&
           "Cannot create pipeline before pipeline layout!");
    assert(renderTarget != nullptr &&
           "Cannot create pipeline for null render target!");

    PipelineConfigInfo pipelineConfigInfo = {};
    Pipeline::defaultPipelineConfig(pipelineConfigInfo);
    pipelineConfigInfo.pipelineLayout = pipelineLayout;

    uint32_t colorAttachmentCount = renderTarget->getColorAttachmentCount() + renderFeatures.size();
    if (pipelineConfigInfo.colorBlendAttachments.size() < colorAttachmentCount) {
      auto first = pipelineConfigInfo.colorBlendAttachments.empty()
                       ? VkPipelineColorBlendAttachmentState{}
                       : pipelineConfigInfo.colorBlendAttachments[0];
      pipelineConfigInfo.colorBlendAttachments.resize(colorAttachmentCount,
                                                      first);
      pipelineConfigInfo.colorBlendInfo.attachmentCount =
          static_cast<uint32_t>(pipelineConfigInfo.colorBlendAttachments.size());
      pipelineConfigInfo.colorBlendInfo.pAttachments =
          pipelineConfigInfo.colorBlendAttachments.data();
    } else {
      pipelineConfigInfo.colorBlendInfo.attachmentCount =
          static_cast<uint32_t>(pipelineConfigInfo.colorBlendAttachments.size());
      pipelineConfigInfo.colorBlendInfo.pAttachments =
          pipelineConfigInfo.colorBlendAttachments.data();
    }

    pipelineConfigInfo.colorAttachmentFormats.clear();
    for (uint32_t i = 0; i < colorAttachmentCount; ++i) {
      if (i == 0) {
        pipelineConfigInfo.colorAttachmentFormats.push_back(
            renderTarget->getColorFormat());
      } else {
        pipelineConfigInfo.colorAttachmentFormats.push_back(VK_FORMAT_R32_UINT);
      }
    }
    pipelineConfigInfo.depthFormat = renderTarget->getDepthFormat();

    pipeline.reset();
    pipeline = make_unique<Pipeline>(shaderInfo.vertFile, shaderInfo.fragFile, pipelineConfigInfo);
  }

  VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
  void createPipelineLayout(const std::vector<VkDescriptorSetLayout> &layouts) override {
    std::vector<VkDescriptorSetLayout> layouts = descriptorSetLayouts;

    VkPushConstantRange pushConstantRange = {};
    pushConstantRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstantRange.offset = 0;
    pushConstantRange.size = sizeof(PushConstantData);

    VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(layouts.size());
    pipelineLayoutInfo.pSetLayouts = layouts.data();
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstantRange;

    VkDevice device = Device::get().device();
    if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr,
                               &pipelineLayout) != VK_SUCCESS)
      throw std::runtime_error("Failed to create pipeline layout!");
  }

  void begin() override{
    if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
      throw std::runtime_error("No command buffer found in FrameInfo!");
    if (FrameInfo::frameIndex < 0 ||
        FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
      throw std::runtime_error("Invalid frame index in FrameInfo!");

    const uint32_t idx = FrameInfo::imageIndex;

    // Transition scene color image to COLOR_ATTACHMENT_OPTIMAL
    VkImageLayout colorLayout = renderTarget->getColorImageLayout(idx);
    if (colorLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) 
      renderTarget->transitionColorImage(
          idx, ImageTransition::ShaderReadToColorOptimal);
    else if (colorLayout != VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) 
      renderTarget->transitionColorImage(
          idx, ImageTransition::UndefinedToColorOptimal);

    VkImageLayout depthLayout = renderTarget->getDepthImageLayout(idx);
    if (depthLayout != VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
      renderTarget->transitionDepthImage(
          idx, ImageTransition::UndefinedToDepthOptimal);

    for (auto &feature : renderFeatures)
      feature->prepare(idx);


    std::vector<VkRenderingAttachmentInfo> colors{};
    colors.emplace_back(renderTarget->getColorAttachment(idx));
    for (auto &feature : renderFeatures)
      feature->pushColorAttachments(colors, idx);

    VkRenderingAttachmentInfo depth = renderTarget->getDepthAttachment(idx);

    VkRenderingInfo renderingInfo = {};
    renderingInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
    renderingInfo.renderArea.offset = {0, 0};
    renderingInfo.renderArea.extent = renderTarget->extent();
    renderingInfo.colorAttachmentCount = colors.size();
    renderingInfo.pColorAttachments = colors.data();
    renderingInfo.pDepthAttachment = &depth;
    renderingInfo.layerCount = 1;

    vkCmdBeginRendering(FrameInfo::commandBuffer, &renderingInfo);

    VkViewport viewport = {};
    viewport.x = 0;
    viewport.y = static_cast<float>(renderTarget->extent().height);
    viewport.width = static_cast<float>(renderTarget->extent().width);
    viewport.height = -static_cast<float>(renderTarget->extent().height);
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(FrameInfo::commandBuffer, 0, 1, &viewport);

    VkRect2D scissor = {};
    scissor.offset = {0, 0};
    scissor.extent = renderTarget->extent();
    vkCmdSetScissor(FrameInfo::commandBuffer, 0, 1, &scissor);
  }
  void record() override {
    pipeline->bind(FrameInfo::commandBuffer);
    auto camSet =
        renderContext->getDescriptorSet(LayoutKey::Camera, FrameInfo::frameIndex);
    if (!camSet.has_value())
      throw std::runtime_error(
          "No Camera descriptor set found for OffscreenRenderer!");
    uint32_t camOffset = rendererId * sizeof(CameraUBO);

    auto lightSet = renderContext->getDescriptorSet(LayoutKey::PointLight,
                                                    FrameInfo::frameIndex);
    if (!lightSet.has_value())
      throw std::runtime_error(
          "No Point light descriptor set found for OffscreenRenderer!");

    VkDescriptorSet sets[] = {camSet.value(), lightSet.value()};
    uint32_t dynamicOffsets[] = {camOffset};

    vkCmdBindDescriptorSets(FrameInfo::commandBuffer,
                            VK_PIPELINE_BIND_POINT_GRAPHICS, getPipelineLayout(),
                            0, 2, sets, 1, dynamicOffsets);
  }
  void end() override {
    if (FrameInfo::commandBuffer == VK_NULL_HANDLE)
      throw std::runtime_error("No command buffer found in FrameInfo!");
    if (FrameInfo::frameIndex < 0 ||
        FrameInfo::frameIndex >= SwapChain::MAX_FRAMES_IN_FLIGHT)
      throw std::runtime_error("Invalid frame index in FrameInfo!");

    vkCmdEndRendering(FrameInfo::commandBuffer);

    const uint32_t idx = FrameInfo::imageIndex;
    #if defined(MAGMA_WITH_EDITOR)
      // Transition scene color to SHADER_READ_ONLY for ImGui sampling
      renderTarget->transitionColorImage(
          idx, ImageTransition::ColorOptimalToShaderRead);
    #else
      renderTarget->transitionColorImage(
          idx, ImageTransition::ColorOptimalToPresent);
    #endif

    for (auto &feature : renderFeatures)
      feature->finish(idx);
  }

  std::vector<std::unique_ptr<RenderFeature>> renderFeatures = {};
  std::unique_ptr<IRenderTarget> renderTarget = nullptr;
  std::unique_ptr<RenderContext> renderContext = nullptr;
  bool isSwapChainDependentFlag = false;

  std::vector<ImTextureID> sceneTextures = {};

  inline static std::unique_ptr<EditorCamera> editorCamera = std::make_unique<EditorCamera>();
};

} // namespace Magma
