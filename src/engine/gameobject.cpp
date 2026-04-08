module;

#include <algorithm>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <typeindex>
#include <unordered_map>
#include <utility>
#include <vector>
#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
#endif

export module engine:gameobject;
import core;

namespace Magma {

namespace util {

inline void sortComponentsByName(std::vector<Component *> &components){
  #if defined(MAGMA_WITH_EDITOR)
  std::sort(components.begin(), components.end(),
            [](Component *a, Component *b) {
              return std::string(a->inspectorName()) <
                     std::string(b->inspectorName());
            });
  #endif
}

}

#if defined(MAGMA_WITH_EDITOR)
  struct GameObjectMenuCallbacks {
      std::function<void(GameObject*)> onLeftClick;   // Inspector::setContext
      std::function<void(GameObject*)> onRightClick;  // SceneMenu::queueContextMenuFor
  };
#endif


/**
 * Entity in the scene that can have multiple components
 * (e.g., Transform, Mesh, Light) and can be part of a hierarchy (
 * parent-child relationship).
 * @note GameObjects are managed by the Scene or its parents.
 */
export class GameObject {
public:
  GameObject *parent = nullptr;
  GameObjectMenuCallbacks callbacks;
  using id_t = uint64_t;

  GameObject(id_t id) : id{id}, name("GameObject_" + std::to_string(id)) {};
  GameObject(id_t id, std::string name) : id{id}, name{name} {};
  GameObject(id_t id, GameObject *parent)
      : id{id}, parent{parent}, name("GameObject_" + std::to_string(id)) {};
  GameObject(id_t id, GameObject *parent, std::string name)
      : id{id}, parent{parent}, name{name} {};

  ~GameObject(){
    components.clear();
    children.clear();
  }

  GameObject(const GameObject &) = delete;
  GameObject &operator=(const GameObject &) = delete;
  GameObject(GameObject &&) = default;
  GameObject &operator=(GameObject &&) = default;

  // ID Management
  GameObject::id_t getNextId() { return nextId++; }

  // Children
  std::vector<GameObject *> getChildren() {
    std::vector<GameObject *> result;
    for (const auto &child : children) {
      if (child)
        result.push_back(child.get());
    }
    return result;
  }

  void addChild() {
    std::unique_ptr<GameObject> child(new GameObject(getNextId(), this));
    children.push_back(std::move(child));
  }
  void addChild(std::unique_ptr<GameObject> child) {
    if (child == nullptr)
      return;

    child->parent = this;
    children.push_back(std::move(child));
  }

  void removeChild(GameObject *child) {
    if (!child) return;
    auto it = std::remove_if(children.begin(), children.end(),
                             [&](const auto &c) { return c.get() == child; });

    if (it == children.end()) return;

    children.erase(it, children.end());
  }

  #if defined(MAGMA_WITH_EDITOR)
    void drawChildren() {
      for (const auto &child : children) {
        if (child) {
          ImGuiTreeNodeFlags flags =
              ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;

          // If no sub-children, display as leaf node
          if (child->children.size() == 0) {
            flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
            ImGui::TreeNodeEx(child->name.c_str(), flags);

            if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && callbacks.onRightClick)
              callbacks.onRightClick(child.get());
            if (ImGui::IsItemClicked() && callbacks.onLeftClick)
              callbacks.onLeftClick(child.get());

            continue;
          }

          // Node with children
          bool open = ImGui::TreeNodeEx(child->name.c_str(), flags);

          if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && callbacks.onRightClick)
            callbacks.onRightClick(child.get());
          if (ImGui::IsItemClicked() && callbacks.onLeftClick)
            callbacks.onLeftClick(child.get());

          if (open) {
            child->drawChildren();
            ImGui::TreePop();
          }
        }
      }
    }
  #endif

  void onUpdate(){
    for (const auto &component : components) {
      if (component.second)
        component.second->onUpdate();
    }

    for (const auto &child : children) {
      if (child)
        child->onUpdate();
    }
  }

  // Movement helpers used by editor input
  void moveRight(float speed) {
    if(auto *transform = getComponent<Transform>())
      transform->position += transform->right() * speed;
  }

  void moveForward(float speed) {
    if(auto *transform = getComponent<Transform>())
      transform->position += transform->forward() * speed;
  }

  void moveUp(float speed) {
    if(auto *transform = getComponent<Transform>())
      transform->position += transform->up() * speed;
  }


  /**
   * Render (recursively)
   * @param renderer SceneRenderer to use for rendering
   * @note This function is called by Scene::onRender() or by the parent
   * GameObject.
   *
  void onRender(SceneRenderer &renderer) {
    for (const auto &component : components) {
      if (component.second)
        component.second->onRender(renderer);
    }

    for (const auto &child : children) {
      if (child)
        child->onRender(renderer);
    }
  }
  */

  RenderProxy collectProxies() const {
    RenderProxy proxy = {};

    for (auto& [typ, component]: components)
      component.collectProxy(proxy);

    return proxy;
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
    util::sortComponentsByName(vec);
    return vec;
  }
  template <typename T, typename... Args>
  T *addComponent(Args &&...args) {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must be a Component");
    auto component = std::make_unique<T>(this, std::forward<Args>(args)...);
    assert(component &&
           "Failed to create component. Make sure the constructor is valid.");

    T *ptr = component.get();

    components[typeid(T)] = std::move(component);
    return ptr;
  }


  id_t id;
  std::string name;

private:
  inline static id_t nextId = 1;

  std::unordered_map<std::type_index, std::unique_ptr<Component>> components;
  std::vector<std::unique_ptr<GameObject>> children;
};


} // namespace Magma
