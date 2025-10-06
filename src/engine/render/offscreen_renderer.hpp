#pragma once
#include "../../core/frame_info.hpp"
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
  VkImage &getSceneImage() const;
  ImVec2 getSceneSize() const;
  ImTextureID getSceneTexture() const {
    return textures[FrameInfo::frameIndex];
  }

  // Textures
  void createOffscreenTextures();

  // Rendering
  void begin() override;
  void record() override;
  void end() override;

  // Resize
  void resize(VkExtent2D newExtent);

private:
  // Textures for ImGui
  std::vector<ImTextureID> textures;
};

} // namespace Magma
