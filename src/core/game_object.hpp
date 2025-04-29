#pragma once
#include "../components/component.hpp"
#include "model.hpp"
#include <any>
#include <glm/ext/matrix_transform.hpp>
#include <typeindex>
#include <unordered_map>

using namespace std;
namespace magma {

class GameObject {
public:
  using id_t = unsigned int;

  GameObject() {}
  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;
  GameObject(GameObject &&) = default;
  GameObject &operator=(GameObject &&) = default;

  static GameObject create() {
    static id_t currentId = 0;
    return GameObject{currentId++};
  }

  template <typename T, typename... Args>
  GameObject &addComponent(Args &&...args) {
    components.emplace(typeid(T), T{this, std::forward<Args>(args)...});

    return *this;
  }

  template <typename T> void removeComponent() { components.erase(typeid(T)); }

  template <typename T> T *getComponent() {
    auto it = components.find(typeid(T));
    if (it != components.end())
      return any_cast<T *>(&it->second);

    return nullptr;
  }

  id_t getId() { return id; }

private:
  id_t id;
  GameObject(id_t id) : id{id} {}

  unordered_map<type_index, Component> components;
  unordered_map<type_index, GameObject> *children;
};
} // namespace magma
