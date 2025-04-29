#pragma once
#include "core/descriptors.hpp"
#include "core/game_object.hpp"
#include "core/object_data.hpp"
#include <cstdint>
#include <memory>

#include <vector>

using namespace std;
namespace magma {

extern const int WIDTH;
extern const int HEIGHT;

class Magma {
public:
  Magma();

  void run();

private:
  Window window{WIDTH, HEIGHT, "Magma Engine"};
  Device device{window};

  unique_ptr<DescriptorPool> descriptorPool;

  std::vector<GameObject> gameObjects{};
  bool wakingUp = true;

  uint32_t frameCounter = 0;
  uint32_t modelIndex = 0;

  void loadGameObjects();
};
} // namespace magma
