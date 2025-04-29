#pragma once

namespace magma {

class GameObject;

struct Component {
  Component(GameObject *parent) : parent(parent){};
  GameObject *parent;
};
} // namespace magma
