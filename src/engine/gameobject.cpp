#include "gameobject.hpp"
#include "scene.hpp"
#include <string>

using namespace std;
namespace Magma {

GameObject::id_t GameObject::nextId = 0;

GameObject::id_t GameObject::getNextId() { return nextId++; }

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

void GameObject::addChild(std::unique_ptr<GameObject> child) {
  if (child == nullptr)
    return;

  child->parent = this;
  children.push_back(std::move(child));
}

std::vector<GameObject *> GameObject::getChildren() {
  std::vector<GameObject *> result;
  for (const auto &child : children) {
    if (child)
      result.push_back(child.get());
  }
  return result;
}

} // namespace Magma
