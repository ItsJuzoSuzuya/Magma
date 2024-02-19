#pragma once

#include "magma_model.hpp"

// libs
#include <glm/gtc/matrix_transform.hpp>

// std
#include <memory>

namespace magma {

struct TransformComponent {
  glm::vec3 position{};
  glm::vec3 scale{1.f, 1.f, 1.f};
  glm::vec3 rotation{};

  glm::mat4 mat4();

  glm::mat3 normalMatrix();
};

class MagmaGameObject {
public:
  using id_t = unsigned int;

  static MagmaGameObject createGameObject() {
    static id_t currentId = 0;
    return MagmaGameObject{currentId++};
  }

  MagmaGameObject(const MagmaGameObject &) = delete;
  MagmaGameObject &operator=(const MagmaGameObject &) = delete;
  MagmaGameObject(MagmaGameObject &&) = default;
  MagmaGameObject &operator=(MagmaGameObject &&) = default;

  id_t getId() { return id; }

  std::shared_ptr<MagmaModel> model{};
  glm::vec3 color{};
  TransformComponent transform{};

private:
  MagmaGameObject(id_t objId) : id{objId} {}

  id_t id;
};
} // namespace magma
