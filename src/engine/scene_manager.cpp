module;
#include <typeinfo>

export module engine:scene_manager;
import :scene;
import :gameobject;

namespace Magma {

class SceneManager {
public:
  static inline Scene *activeScene;

  template <typename T> T *getComponentFromGameObject(GameObject::id_t id) const {
    static_assert(std::is_base_of<Component, T>::value,
                  "T must be a Component");

    GameObject *obj = findGameObjectById(id);
    auto components = obj.getComponents();

    auto it = components.find(typeid(T));
    if (it != components.end())
      return static_cast<T *>(it->second.get());
    return nullptr;
  }

  static GameObject *findGameObjectById(GameObject::id_t id){
    if (activeScene == nullptr)
      return nullptr;

    std::function<GameObject *(GameObject *)> findObjectInChildren =
        [&](GameObject *node) -> GameObject * {
      if (!node)
        return nullptr;

      if (node->id == id)
        return node;

      auto children = node->getChildren();
      for (auto *child : children) {
        if (auto obj = findObjectInChildren(child))
          return obj;
      }
      return nullptr;
    };

    for (const auto &go : activeScene->gameObjects) {
      if (!go) 
        continue;

      if (go->id == id) 
        return go.get();

      if (auto obj = findObjectInChildren(go.get())) 
        return obj;
    }
    return nullptr;
  }

  #if defined(MAGMA_WITH_EDITOR)
    static void drawSceneTree(){
      if (activeScene == nullptr)
        return;

      for (auto &gameObject : activeScene->gameObjects) {
        if (gameObject == nullptr)
          continue;

        ImGuiTreeNodeFlags flags =
            ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanFullWidth;

        // If no children, display as leaf node
        if (!gameObject->hasChildren()) {
          flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
          ImGui::TreeNodeEx(gameObject->name.c_str(), flags);

          if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && gameObject.callbacks.onRightClick)
            gameObject.callback.onRightClick(gameObject.get())
          if (ImGui::IsItemClicked() && gameObject.callbacks.onLeftClick)
            gameObject.callbacks.onLeftClick(gameObject.get());

          continue;
        }

        // Else display as tree node
        bool open = ImGui::TreeNodeEx(gameObject->name.c_str(), flags);

        if (ImGui::IsItemClicked(ImGuiMouseButton_Right) && gameObject.callbacks.onRightClick)
          gameObject.callback.onRightClick(gameObject.get())
        if (ImGui::IsItemClicked() && gameObject.callbacks.onLeftClick)
          gameObject.callbacks.onLeftClick(gameObject.get());

        if (open) {
          gameObject->drawChildren();
          ImGui::TreePop();
        }
      }
    }
  #endif
};

}

