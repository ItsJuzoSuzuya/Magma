#pragma once
#include "engine/gameobject.hpp"
#include "engine/scene_manager.hpp"
#include <memory>
#include <string>

namespace Magma {

struct Project {
  Scene* scene = nullptr;
  GameObject *camera = nullptr;
  std::string path = "";
};


} // namespace Magma
