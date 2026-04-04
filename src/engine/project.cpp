module;
#include <memory>
#include <string>

module engine:project;
import :scene;

namespace Magma {

export struct Project {
  std::unique_ptr<Scene> scene = nullptr;
  std::string path = "";
};

}

