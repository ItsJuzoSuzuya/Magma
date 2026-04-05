module;

#include <functional>
#if defined(MAGMA_WITH_EDITOR)
  #include "imgui.h"
#endif

module engine:scene;
import :gameobject;

namespace Magma {

export class Scene {
public:
  Scene() {
    if (activeScene == nullptr)
      setActive();
  }

  ~Scene(){
    Device::waitIdle();

    if (activeScene == this)
      activeScene = nullptr;
  }

  Scene(const Scene &) = delete;
  Scene &operator=(const Scene &) = delete;
  Scene(Scene &&) = default;
  Scene &operator=(Scene &&) = default;

  std::vector<std::unique_ptr<GameObject>> &getGameObjects(){
    return gameObjects;
  }
  GameObject *findGameObjectById(GameObject::id_t id){
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
  GameObject &addGameObject(std::unique_ptr<GameObject> gameObject, bool hidden = false){

    assert(gameObject != nullptr &&
           "GameObject cannot be null when adding to scene");

    GameObject &ref = *gameObject;
    gameObjects.push_back(std::move(gameObject));
    return ref;
  }
  void removeGameObject(GameObject *gameObject){
    defer(SceneAction::remove(gameObject));
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

  /**
   * Defers an action to be executed after the current frame
   * @param func The function to be executed
   * @note This is useful for actions that modify the scene
   */
  void defer(std::function<void()> func) { deferredActions.push_back(func); }

  /**
   * Process deferred actions queued during the frame
   * @note This should be called at the end of each frame
   */
  void processDeferredActions(){
    if (activeScene == nullptr)
      return;
    if (deferredActions.empty())
      return;

    Device::waitIdle();
    for (auto &action : deferredActions)
      action();

    deferredActions.clear();
  }


private:
  std::vector<std::unique_ptr<GameObject>> gameObjects;
  std::vector<std::function<void()>> deferredActions;
};

}
