#pragma once
#include "../../core/renderer.hpp"
#include <cstdint>

namespace Magma {

class RenderTargetInfo;

class OffscreenRenderer : public Renderer {
public:
  // Constructor
  OffscreenRenderer(Device &device, RenderTargetInfo &info,
                    VkDescriptorSetLayout descriptorSetLayout);
  // Destructor
  ~OffscreenRenderer();

  // Getters
  VkImage &getSceneImage(uint32_t frameIndex) const;
  ImVec2 getSceneSize() const;
  ImTextureID getSceneTexture(uint32_t frameIndex) const {
    return textures[frameIndex];
  }

  // Textures
  void createOffscreenTextures();

  // Rendering
  void begin(VkCommandBuffer commandBuffer, uint32_t frameIndex) override;
  void record(VkCommandBuffer commandBuffer) override;
  void end(VkCommandBuffer commandBuffer) override;

private:
  // Textures for ImGui
  std::vector<ImTextureID> textures;
};

} // namespace Magma
