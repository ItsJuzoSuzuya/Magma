#include "gameobject.hpp"
#include "imgui.h"
#include "scene.hpp"
#include "widgets/scene_tree.hpp"
#include <memory>
#include <print>
#include <string>

using namespace std;
namespace Magma {

// Static member initialization
GameObject::id_t GameObject::nextId = 0;

GameObject::id_t GameObject::getNextId() { return nextId++; }

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

void GameObject::drawChildren() {
  for (const auto &child : children) {
    if (child) {

      ImGuiTreeNodeFlags flags =
          ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;

      // If no sub-children, display as leaf node
      if (!child->hasChildren()) {
        flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
        ImGui::TreeNodeEx(child->name.c_str(), flags);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
          SceneTree::setContextTarget(child.get());
          ImGui::OpenPopup("SceneMenu");
        }

        continue;
      }

      // Node with children
      bool open = ImGui::TreeNodeEx(child->name.c_str(), flags);

      if (ImGui::IsItemClicked(ImGuiMouseButton_Right)) {
        SceneTree::setContextTarget(child.get());
        ImGui::OpenPopup("SceneMenu");
      }

      if (open) {
        child->drawChildren();
        ImGui::TreePop();
      }
    }
  }
}

} // namespace Magma
