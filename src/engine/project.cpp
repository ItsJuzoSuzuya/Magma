module;
#include <memory>
#include <string>

export module engine:project;
import :scene;
import :gameobject;

namespace Magma {

export struct Project {
  std::unique_ptr<Scene> scene = nullptr;
  GameObject *camera = nullptr;
  std::string path = "";
};

}
