#pragma once

#include "engine/gameobject.hpp"
#include "engine/render/features/object_picker.hpp"
#include "engine/render/scene_renderer.hpp"
#include "imgui.h"
#include <cstdint>
#include <functional>
#include <optional>
#include <vulkan/vulkan_core.h>

namespace Magma {

struct Viewport {
  std::function<ImTextureID(int frameIndex)>  getTexture;
  std::function<ImVec2()>                     getSize;
  std::function<void(VkExtent2D newExtent)>   onResize;

  struct Picker {
    std::function<void(uint32_t x, uint32_t y)> request;              
    std::function<GameObject*()>                poll;                 
  };
  std::optional<Picker> picker;
};

inline Viewport makeViewport(SceneRenderer *r, bool hasPicker){
  std::optional<Viewport::Picker> picker = std::nullopt;
  if (hasPicker){
    picker = {
      .request = [r](uint32_t x, uint32_t y) {   
        r->getFeature<ObjectPicker>().requestPick(x, y); },
      .poll = [r]() {
        return r->getFeature<ObjectPicker>().pollPickResult(); }
    };
  }

  return {
    .getTexture = [r](int f) { return r->getSceneTexture(f); },        
    .getSize = [r]() { return r->getSceneSize(); },           
    .onResize = [r](VkExtent2D e) { r->onResize(e); },
    .picker = picker 
  };
}

}
