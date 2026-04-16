#include "gameobject.hpp"
#include <algorithm>
#include <memory>

#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
  #include "widgets/scene_menu.hpp"
#endif

namespace Magma {

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
  std::unique_ptr<GameObject> child(this);
  children.push_back(std::move(child));
}

void GameObject::addChild(std::unique_ptr<GameObject> child) {
  if (!child) return;
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
    if (!child) continue;

    ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;

    if (!child->hasChildren()) {
      flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
      ImGui::TreeNodeEx(child->name.c_str(), flags);

      if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && callbacks.onRightClick)
        callbacks.onRightClick(child.get());
      if (ImGui::IsItemClicked() && callbacks.onLeftClick)
        callbacks.onLeftClick(child.get());
      continue;
    }

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
#endif

void GameObject::onUpdate() {
  for (const auto &[type, component] : components) {
    if (component)
      component->onUpdate();
  }
  for (const auto &child : children) {
    if (child)
      child->onUpdate();
  }
}

RenderProxy GameObject::collectProxies() const {
  RenderProxy proxy = {};
  for (auto &[type, component] : components)
    component->collectProxy(proxy);
  return proxy;
}

} // namespace Magma
