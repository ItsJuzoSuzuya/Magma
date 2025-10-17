#pragma once
#include "components/component.hpp"
#include <memory>
#include <print>
#include <typeindex>
#include <unordered_map>
#include <vector>
namespace Magma {

class Renderer;

/**
 * Entity in the scene that can have multiple components
 * (e.g., Transform, Mesh, Light) and can be part of a hierarchy (
 * parent-child relationship).
 * @note GameObjects are managed by the Scene or its parents.
 */
class GameObject {
public:
  using id_t = uint64_t;

  // Destructor
  ~GameObject();
  void destroy();

  // Factory methods
  static GameObject &create();
  static GameObject &create(std::string name);
  static GameObject &create(GameObject &parent);
  static GameObject &create(GameObject &parent, std::string name);

  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;
  GameObject(GameObject &&) = default;
  GameObject &operator=(GameObject &&) = default;

  // Getters
  static id_t getNextId();
  std::vector<GameObject *> getChildren();

  // Component
  template <typename T, typename... Args>
  GameObject &addComponent(Args &&...args) {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must be a Component");
    auto component = std::make_unique<T>(this, std::forward<Args>(args)...);
    std::println("Added component of type {}", typeid(T).name());
    components[typeid(T)] = std::move(component);
    return *this;
  }
  template <typename T> T *getComponent() const {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must be a Component");
    auto it = components.find(typeid(T));
    if (it != components.end())
      return static_cast<T *>(it->second.get());
    return nullptr;
  }
  std::vector<Component *> getComponents() const {
    std::vector<Component *> vec = {};
    for (auto &[type, comp] : components)
      vec.push_back(comp.get());
    return vec;
  }

  // Parent
  GameObject *parent = nullptr;

  // Children
  void addChild();
  void addChild(std::unique_ptr<GameObject> child);
  void drawChildren();
  bool hasChildren() const { return !children.empty(); }

  /**
   * Render (recursive)
   * @param renderer Renderer to use for rendering
   * @note This function is called by Scene::onRender() or by the parent
   * GameObject.
   */
  void onRender(Renderer &renderer);
  void draw();

  id_t id;
  std::string name;

private:
  static id_t nextId;

  GameObject(id_t id) : id{id}, name("GameObject " + std::to_string(id)) {};
  GameObject(id_t id, std::string name) : id{id}, name{name} {};
  GameObject(id_t id, GameObject *parent)
      : id{id}, parent{parent}, name("GameObject " + std::to_string(id)) {};
  GameObject(id_t id, GameObject *parent, std::string name)
      : id{id}, parent{parent}, name{name} {};

  // Hierarchy
  std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
  std::vector<std::unique_ptr<GameObject>> children;
};

} // namespace Magma
