#pragma once
#include "../components/component.hpp"
#include "model.hpp"
#include <glm/ext/matrix_transform.hpp>
#include <iostream>
#include <memory>
#include <type_traits>
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

  static GameObject create();

  template <typename T, typename... Args>
  GameObject &addComponent(Args &&...args) {
    shared_ptr<T> component = nullptr;
    if constexpr (is_same_v<T, Model>)
      component = Model::createModelFromFile(this, std::forward<Args>(args)...);
    else
      component = make_shared<T>(this, std::forward<Args>(args)...);

    components[typeid(T)] = component;
    return *this;
  }

  template <typename T> void removeComponent() { components.erase(typeid(T)); }

  template <typename T> shared_ptr<T> getComponent() {
    auto it = components.find(typeid(T));
    if (it == components.end())
      return nullptr;

    try {
      return dynamic_pointer_cast<T>(it->second);
    } catch (const std::bad_cast &e) {
      cout << "Error: " << e.what() << endl;
      return nullptr;
    }
  }

  id_t getId() { return id; }

private:
  id_t id;
  GameObject(id_t id) : id{id} {}

  unordered_map<type_index, shared_ptr<Component>> components;
  unordered_map<type_index, GameObject> *children;
};
} // namespace magma
