#include "render_system.hpp"
#include "core/device.hpp"
#include "core/window.hpp"
#include "deletion_queue.hpp"
#include "engine/scene.hpp"
#include "engine/time.hpp"
#include "renderer.hpp"
#include "swapchain.hpp"
#include <vulkan/vulkan_core.h>

#if defined(MAGMA_WITH_EDITOR)
  #include "engine/widgets/dock_layout.hpp"
  #include "engine/widgets/game_editor.hpp"
  #include "engine/widgets/game_view.hpp"
  #include "engine/widgets/inspector.hpp"
  #include "engine/widgets/runtime_control.hpp"
  #include "engine/widgets/scene_tree.hpp"
  #include "imgui.h"
  #include "imgui_impl_glfw.h"
  #include "imgui_impl_vulkan.h"
#endif

#include <GLFW/glfw3.h>
#include <cassert>
#include <memory>

namespace Magma {

RenderSystem:: RenderSystem(Window &window) : window{window} {
  device = std::make_unique<Device>(window);
  swapChain = std::make_unique<SwapChain>(window.getExtent());
  renderContext = std::make_unique<RenderContext>();

  #if defined(MAGMA_WITH_EDITOR)
    RenderTargetInfo offscreenInfo = swapChain->getRenderInfo();
    offscreenInfo.extent.width /= 2;
    offscreenInfo.extent.height /= 2;
    offscreenInfo.imageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
    
    // Renderers register themselves to the render context
    offscreenRendererEditor = std::make_unique<OffscreenRenderer>(
        offscreenInfo, renderContext.get(), RendererMode::Editor);
    offscreenRendererGame = std::make_unique<OffscreenRenderer>(
        offscreenInfo, renderContext.get(), RendererMode::Game);

    editorCamera = std::make_unique<EditorCamera>();
    offscreenRendererEditor->setActiveCamera(editorCamera->getCamera());

    imguiRenderer = std::make_unique<ImGuiRenderer>(*swapChain);
    imguiRenderer->addWidget(std::make_unique<RuntimeControl>());
    imguiRenderer->addWidget(std::make_unique<SceneTree>());
    imguiRenderer->addWidget(std::make_unique<Inspector>());

    imguiRenderer->addWidget(std::make_unique<GameEditor>(
        *offscreenRendererEditor.get(), editorCamera.get()));
    imguiRenderer->addWidget(std::make_unique<GameView>(*offscreenRendererGame.get()));
  #else
    offscreenRenderer = std::make_unique<OffscreenRenderer>(
        *swapChain, renderContext.get());
  #endif

  createCommandBuffers();
}

RenderSystem::~RenderSystem() {
  Device::waitIdle();

  renderContext.reset();


  renderContext.reset();
  #if defined(MAGMA_WITH_EDITOR)
    if (offscreenRendererEditor)
      offscreenRendererEditor.reset();
    if (offscreenRendererGame)
      offscreenRendererGame.reset();

    ImGui_ImplVulkan_Shutdown();

    if (imguiRenderer)
      imguiRenderer.reset();

    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
  #else
    if (offscreenRenderer)
      offscreenRenderer.reset();
  #endif

  DeletionQueue::flushAll();
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

void RenderSystem::renderFrame() {
  Time::update(glfwGetTime());

  #if defined(MAGMA_WITH_EDITOR)
    if (firstFrame) {
      offscreenRendererEditor->createOffscreenTextures();
      offscreenRendererGame->createOffscreenTextures();
    }
  #endif

  if (beginFrame()) {
    #if defined(MAGMA_WITH_EDITOR)
      {
        offscreenRendererEditor->begin();
        offscreenRendererEditor->record();
        editorCamera->onUpdate();
        editorCamera->onRender(*offscreenRendererEditor);
        Scene::onRender(*offscreenRendererEditor);
        offscreenRendererEditor->end();
      }

      {
        Camera *mainCam = Scene::getActiveCamera();
        offscreenRendererGame->setActiveCamera(
            mainCam ? mainCam : editorCamera->getCamera());

        offscreenRendererGame->begin();
        offscreenRendererGame->record();
        Scene::onRender(*offscreenRendererGame);
        offscreenRendererGame->end();
      }

      {
        imguiRenderer->begin();
        imguiRenderer->record();
        imguiRenderer->end();
      }
    #else
      {
        Camera *mainCam = Scene::getActiveCamera();
        offscreenRenderer->setActiveCamera(mainCam);
      }
      offscreenRenderer->begin();
      offscreenRenderer->record();
      Scene::onRender(*offscreenRenderer);
      offscreenRenderer->end();
    #endif

    endFrame();
  }

  if (firstFrame)
    firstFrame = false;
}

#if defined(MAGMA_WITH_EDITOR)
  ImGui_ImplVulkan_InitInfo RenderSystem::getImGuiInitInfo() {
    ImGui_ImplVulkan_InitInfo init_info = {};
    device->populateImGuiInitInfo(&init_info);
    init_info.ApiVersion = VK_API_VERSION_1_3;
    init_info.DescriptorPool = imguiRenderer->getDescriptorPool();
    init_info.DescriptorPoolSize = 0;

    init_info.RenderPass = VK_NULL_HANDLE;
    init_info.Subpass = 0;

    // Store color format for dynamic rendering
    // @note This is needed because ImGui doesnt store the color format correctly
    // on Arch Linux systems
    imguiColorFormat = swapChain->getRenderInfo().colorFormat;

    init_info.UseDynamicRendering = true;
    init_info.PipelineRenderingCreateInfo = {};
    init_info.PipelineRenderingCreateInfo.sType =
        VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
    init_info.PipelineRenderingCreateInfo.colorAttachmentCount = 1;
    init_info.PipelineRenderingCreateInfo.pColorAttachmentFormats = &imguiColorFormat ;
    init_info.PipelineRenderingCreateInfo.depthAttachmentFormat =
        swapChain->getRenderInfo().depthFormat;
    init_info.PipelineRenderingCreateInfo.stencilAttachmentFormat =
        VK_FORMAT_UNDEFINED;

    init_info.MinImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
    init_info.ImageCount = SwapChain::MAX_FRAMES_IN_FLIGHT;
    init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
    init_info.Allocator = nullptr;
    init_info.CheckVkResultFn = nullptr;

    return init_info;
  }
#endif

// ----------------------------------------------------------------------------
// Private Methods
// ----------------------------------------------------------------------------

void RenderSystem::createCommandBuffers() {
  commandBuffers.resize(SwapChain::MAX_FRAMES_IN_FLIGHT);

  VkCommandBufferAllocateInfo allocInfo = {};
  allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
  allocInfo.commandPool = device->getCommandPool();
  allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
  allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

  if (vkAllocateCommandBuffers(device->device(), &allocInfo,
                               commandBuffers.data()) != VK_SUCCESS)
    throw std::runtime_error("Failed to allocate command buffers!");
}

void RenderSystem::recreateSwapChain(VkExtent2D extent) {
  Device::waitIdle();

  std::shared_ptr<SwapChain> oldSwapChain = std::move(swapChain);
  swapChain = std::make_unique<SwapChain>(extent, oldSwapChain);
}

// Rendering
bool RenderSystem::beginFrame() {
  #if defined(MAGMA_WITH_EDITOR)
    imguiRenderer->newFrame();
    imguiRenderer->preFrame();
  #endif

  auto result = swapChain->acquireNextImage();

  /* VK_ERROR_OUT_OF_DATE_KHR indicates that the swap chain is no longer
     compatible with the surface and needs to be recreated. This can happen
      e.g. after a window resize. */
  if (result == VK_ERROR_OUT_OF_DATE_KHR) {
    onWindowResize();
    return false;
  } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
    throw std::runtime_error("Failed to acquire next swap chain image!");

  VkCommandBuffer commandBuffer = commandBuffers[FrameInfo::frameIndex];
  VkCommandBufferBeginInfo beginInfo = {};
  beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

  if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS)
    throw std::runtime_error("Failed to begin recording command buffer!");

  FrameInfo::commandBuffer = commandBuffer;

  return true;
}

void RenderSystem::endFrame() {
  if (vkEndCommandBuffer(FrameInfo::commandBuffer) != VK_SUCCESS)
    throw std::runtime_error("Failed to record command buffer!");

  auto result = swapChain->submitCommandBuffer(&FrameInfo::commandBuffer);

  if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR ||
      window.wasWindowResized()) {
    onWindowResize();
    return;
  } else if (result != VK_SUCCESS)
    throw std::runtime_error("Failed to present swap chain image!");

  FrameInfo::advanceFrame();
  Scene::current()->processDeferredActions();
  DeletionQueue::flushForFrame(FrameInfo::frameIndex);
}

void RenderSystem::onWindowResize() {
  auto extent = window.getExtent();
  while (extent.width == 0 || extent.height == 0) {
    extent = window.getExtent();
    glfwWaitEvents();
  }

  window.resetWindowResizedFlag();
  recreateSwapChain(extent);

  #if defined(MAGMA_WITH_EDITOR)
    imguiRenderer->resize(swapChain->getRenderInfo().extent,
                          swapChain->getSwapChain());
  #else
    offscreenRenderer->resize(swapChain->getRenderInfo().extent,
                              swapChain->getSwapChain());
  #endif
}

// Dockspace 
#if defined(MAGMA_WITH_EDITOR)
  void RenderSystem::createDockspace(ImGuiID &dockspace_id, const ImVec2 &size) {
    DockLayout dockLayout(dockspace_id, size);

    ImGuiID dock_id_left = dockLayout.splitLeft(0.25f);
    ImGuiID dock_id_right = dockLayout.splitRight(0.25f);
    ImGuiID dock_id_center = dockLayout.centerNode();

    dockLayout.makeCentral();

    dockLayout.dockWindow("Scene Tree", dock_id_left);
    dockLayout.dockWindow("Editor", dock_id_center);
    dockLayout.dockWindow("Inspector", dock_id_right);

    dockLayout.finish();
  }
#endif

} // namespace Magma
