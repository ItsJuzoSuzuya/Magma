#include "gameobject.hpp"
#include "components/mesh.hpp"
#include "scene.hpp"

#if defined(MAGMA_WITH_EDITOR)
#include "imgui.h"
#include "widgets/inspector.hpp"
#include "widgets/scene_menu.hpp"
#endif

#include <memory>
#include <string>

using namespace std;
namespace Magma {

// Static member initialization
GameObject::id_t GameObject::nextId = 1;

GameObject::id_t GameObject::getNextId() { return nextId++; }

// Destructor
GameObject::~GameObject() {
  components.clear();
  children.clear();
}

void GameObject::destroy() {
  Scene::current()->removeGameObject(this);

  #if defined(MAGMA_WITH_EDITOR)
    Inspector::setContext(nullptr);
  #endif
}

// Factory methods
GameObject &GameObject::create() {
  auto obj = unique_ptr<GameObject>(new GameObject(getNextId()));
  GameObject &ref = *obj;
  Scene::current()->addGameObject(std::move(obj));
  return ref;
}

GameObject &GameObject::create(string name) {
  auto obj = unique_ptr<GameObject>(new GameObject(getNextId(), name));
  GameObject &ref = *obj;
  Scene::current()->addGameObject(std::move(obj));
  return ref;
}

GameObject &GameObject::create(GameObject &parent) {
  auto obj = unique_ptr<GameObject>(new GameObject(getNextId(), &parent));
  GameObject &ref = *obj;
  parent.addChild(std::move(obj));
  return ref;
}

GameObject &GameObject::create(GameObject &parent, string name) {
  auto obj = unique_ptr<GameObject>(new GameObject(getNextId(), &parent, name));
  GameObject &ref = *obj;
  parent.addChild(std::move(obj));
  return ref;
}

// Children
void GameObject::addChild() {
  unique_ptr<GameObject> child(new GameObject(getNextId(), this));
  children.push_back(std::move(child));
}

void GameObject::addChild(unique_ptr<GameObject> child) {
  if (child == nullptr)
    return;

  child->parent = this;
  children.push_back(std::move(child));
}

vector<GameObject *> GameObject::getChildren() {
  vector<GameObject *> result;
  for (const auto &child : children) {
    if (child)
      result.push_back(child.get());
  }
  return result;
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
void GameObject::onRender(Renderer &renderer) {
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
