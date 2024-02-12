#pragma once

#include "magma_model.hpp"
#include <glm/fwd.hpp>
#include <glm/trigonometric.hpp>
#include <memory>
namespace magma {

struct Transform2dComponent {
  glm::vec2 position{};
  glm::vec2 scale{1.f, 1.f};
  float rotation;

  glm::mat2 mat2() {
    const float s = glm::sin(rotation);
    const float c = glm::cos(rotation);

    glm::mat2 rotateMat{{c, s}, {-s, c}};
    glm::mat2 scaleMat{{scale.x, .0f}, {.0f, scale.y}};

    return rotateMat * scaleMat;
  }
};

class MagmaGameObject {
public:
  using id_t = unsigned int;

  static MagmaGameObject createGameObject() {
    static id_t currentId = 0;
    return MagmaGameObject(currentId++);
  }

  MagmaGameObject(const MagmaGameObject &) = delete;
  MagmaGameObject &operator=(const MagmaGameObject &) = delete;
  MagmaGameObject(MagmaGameObject &&) = default;
  MagmaGameObject &operator=(MagmaGameObject &&) = default;

  const id_t getId() { return id; }

  std::shared_ptr<MagmaModel> model{};
  glm::vec3 color{};
  Transform2dComponent transform2D{};

private:
  MagmaGameObject(id_t objId) : id{objId} {}

  id_t id;
};
} // namespace magma
