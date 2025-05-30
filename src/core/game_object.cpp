#include "game_object.hpp"
#include "model.hpp"
#include <iostream>

namespace magma {

GameObject GameObject::create() {
  static id_t currentId = 0;
  return GameObject{currentId++};
}

} // namespace magma
