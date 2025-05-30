#pragma once

#include <memory>
using namespace std;
namespace magma {

class GameObject;

class Component {
public:
  virtual ~Component() = default;

  GameObject *parent;
  virtual void setParent(GameObject *parent) { this->parent = parent; }
  virtual GameObject *getParent() const { return parent; }
};

} // namespace magma
