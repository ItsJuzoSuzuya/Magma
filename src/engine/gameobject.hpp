#pragma once
#include "components/component.hpp"
#include "components/transform.hpp"
#include <algorithm>
#include <cassert>
#include <cstdint>
#include <functional>
#include <memory>
#include <print>
#include <string>
#include <typeindex>
#include <unordered_map>
#include <vector>

namespace Magma {

namespace util {
inline void sortComponentsByName(std::vector<Component *> &components) {
  #if defined(MAGMA_WITH_EDITOR)
  std::sort(components.begin(), components.end(),
            [](Component *a, Component *b) {
              return std::string(a->inspectorName()) < std::string(b->inspectorName());
            });
  #endif
}
} // namespace util

/**
 * Entity in the scene that can have multiple components and children.
 */
class GameObject {
public:
  using id_t = uint64_t;

  GameObject() : id{getNextId()}, name("GameObject_" + std::to_string(id)) {}
  GameObject(std::string name) : id{getNextId()}, name{name} {}
  GameObject(GameObject *parent) : id{getNextId()}, parent{parent}, name("GameObject_" + std::to_string(id)) {}
  GameObject(GameObject *parent, std::string name) : id{getNextId()}, parent{parent}, name{name} {}

  ~GameObject() {
    components.clear();
    children.clear();
  }

  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;
  GameObject(GameObject &&) = default;
  GameObject &operator=(GameObject &&) = default;

  // Children
  std::vector<GameObject *> getChildren();
  void addChild();
  void addChild(std::unique_ptr<GameObject> child);
  void removeChild(GameObject *child);
  bool hasChildren() const { return !children.empty(); }

  #if defined(MAGMA_WITH_EDITOR)
    struct GameObjectMenuCallbacks {
      std::function<void(GameObject*)> onLeftClick;
      std::function<void(GameObject*)> onRightClick;
    };
    GameObjectMenuCallbacks callbacks;
    void drawChildren();
  #endif

  void onUpdate();

  // Movement helpers used by editor input
  void moveRight(float speed) {
    if (auto *t = getComponent<Transform>()) t->position += t->right() * speed;
  }
  void moveForward(float speed) {
    if (auto *t = getComponent<Transform>()) t->position += t->forward() * speed;
  }
  void moveUp(float speed) {
    if (auto *t = getComponent<Transform>()) t->position += t->up() * speed;
  }

  // Collect all component proxies for rendering
  RenderProxy collectProxies() const;

  template <typename T>
  T *getComponent() const {
    static_assert(std::is_base_of<Component, T>::value, "T must be a Component");
    auto it = components.find(typeid(T));
    if (it != components.end())
      return static_cast<T *>(it->second.get());
    return nullptr;
  }

  std::vector<Component *> getComponents() const {
    std::vector<Component *> vec;
    for (auto &[type, comp] : components)
      vec.push_back(comp.get());
    util::sortComponentsByName(vec);
    return vec;
  }

  template <typename T, typename... Args, typename std::enable_if<!std::is_same<T, Transform>::value, std::size_t>::type = 0>
  T *addComponent(Args &&...args) {
    static_assert(std::is_base_of<Component, T>::value, "T must be a Component");

    std::unique_ptr<T> component = nullptr;
    component = std::make_unique<T>(std::forward<Args>(args)...);
    assert(component && "Failed to create component.");

    T *ptr = component.get();
    components[typeid(T)] = std::move(component);

    return ptr;
  }

  template <typename T, typename std::enable_if<std::is_same<T, Transform>::value, std::size_t>::type = 0>
  T *addComponent() {
    static_assert(std::is_base_of<Component, T>::value, "T must be a Component");

    std::unique_ptr<T> component = nullptr;
    component = std::make_unique<T>(static_cast<uint32_t>(id));
    assert(component && "Failed to create component.");

    T *ptr = component.get();
    components[typeid(T)] = std::move(component);

    return ptr;
  }

  id_t id;
  std::string name;
  GameObject *parent = nullptr;

private:
  inline static id_t nextId = 1;
  id_t getNextId() { return nextId++; }

  std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
  std::vector<std::unique_ptr<GameObject>> children;
};



} // namespace Magma
