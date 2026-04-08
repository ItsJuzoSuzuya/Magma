module;
#include <memory>
#include <string>

export module engine:project;
import :scene;

namespace Magma {

export struct Project {
  std::unique_ptr<Scene> scene = nullptr;
  GameObject camera;
  std::string path = "";
};

}

