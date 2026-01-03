#include "gameobject.hpp"
#include "components/mesh.hpp"
#include "scene.hpp"
#include <algorithm>
#include <memory>
#include <string>

#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
  #include "widgets/inspector.hpp"
  #include "widgets/scene_menu.hpp"
#endif

namespace Magma {

GameObject::~GameObject() {
  components.clear();
  children.clear();
}

// ----------------------------------------------------------------------------
// Public Methods
// ----------------------------------------------------------------------------

// Factory methods
GameObject &GameObject::create() {
  auto obj = std::unique_ptr<GameObject>(new GameObject(getNextId()));
  GameObject &ref = *obj;
  Scene::current()->addGameObject(std::move(obj));
  return ref;
}

GameObject &GameObject::create(std::string name) {
  auto obj = std::unique_ptr<GameObject>(new GameObject(getNextId(), name));
  GameObject &ref = *obj;
  Scene::current()->addGameObject(std::move(obj));
  return ref;
}

GameObject &GameObject::create(GameObject &parent) {
  auto obj = std::unique_ptr<GameObject>(new GameObject(getNextId(), &parent));
  GameObject &ref = *obj;
  parent.addChild(std::move(obj));
  return ref;
}

GameObject &GameObject::create(GameObject &parent, std::string name) {
  auto obj = std::unique_ptr<GameObject>(new GameObject(getNextId(), &parent, name));
  GameObject &ref = *obj;
  parent.addChild(std::move(obj));
  return ref;
}

// Cleanup
void GameObject::destroy() {
  Scene::current()->removeGameObject(this);

  #if defined(MAGMA_WITH_EDITOR)
    Inspector::setContext(nullptr);
  #endif
}

// ID Management
GameObject::id_t GameObject::getNextId() { return nextId++; }

// Children
std::vector<GameObject *> GameObject::getChildren() {
  std::vector<GameObject *> result;
  for (const auto &child : children) {
    if (child)
      result.push_back(child.get());
  }
  return result;
}

void GameObject::addChild() {
  std::unique_ptr<GameObject> child(new GameObject(getNextId(), this));
  children.push_back(std::move(child));
}

void GameObject::addChild(std::unique_ptr<GameObject> child) {
  if (child == nullptr)
    return;

  child->parent = this;
  children.push_back(std::move(child));
}

void GameObject::removeChild(GameObject *child) {
  if (!child) return;
  auto it = std::remove_if(children.begin(), children.end(),
                           [&](const auto &c) { return c.get() == child; });

  if (it == children.end()) return;

  children.erase(it, children.end());
}

#if defined(MAGMA_WITH_EDITOR)
  void GameObject::drawChildren() {
    for (const auto &child : children) {
      if (child) {
        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;

        // If no sub-children, display as leaf node
        if (!child->hasChildren()) {
          flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
          ImGui::TreeNodeEx(child->name.c_str(), flags);

          if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
            SceneMenu::queueContextMenuFor(child.get());
          if (ImGui::IsItemClicked())
            Inspector::setContext(child.get());

          continue;
        }

        // Node with children
        bool open = ImGui::TreeNodeEx(child->name.c_str(), flags);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right))
          SceneMenu::queueContextMenuFor(child.get());
        if (ImGui::IsItemClicked())
          Inspector::setContext(child.get());

        if (open) {
          child->drawChildren();
          ImGui::TreePop();
        }
      }
    }
  }
#endif

// Rendering
void GameObject::onRender(SceneRenderer &renderer) {
  for (const auto &component : components) {
    if (component.second)
      component.second->onRender(renderer);
  }

  draw();

  for (const auto &child : children) {
    if (child)
      child->onRender(renderer);
  }
}

void GameObject::draw() {
  auto mesh = getComponent<Mesh>();
  if (mesh)
    mesh->draw();
}

} // namespace Magma
