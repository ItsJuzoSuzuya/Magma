module;

module engine:scene_manager;
import :scene;
import :gameobject;

namespace Magma {

export class SceneManager {
public:
  inline static Scene *activeScene = nullptr;
  inline static GameObject* activeCamera;

  GameObject &createGameObject() {
    auto obj = std::unique_ptr<GameObject>(new GameObject(getNextId()));
    GameObject &ref = *obj;
    activeScene->addGameObject(std::move(obj));
    return ref;
  }

  GameObject &createGameObjec(std::string name) {
    auto obj = std::unique_ptr<GameObject>(new GameObject(getNextId(), name));
    GameObject &ref = *obj;
    activeScene->addGameObject(std::move(obj));
    return ref;
  }

  GameObject &createGameObject(GameObject &parent) {
    auto obj = std::unique_ptr<GameObject>(new GameObject(getNextId(), &parent));
    GameObject &ref = *obj;
    parent.addChild(std::move(obj));
    return ref;
  }

  GameObject &createGameObject(GameObject &parent, std::string name) {
    auto obj = std::unique_ptr<GameObject>(new GameObject(getNextId(), &parent, name));
    GameObject &ref = *obj;
    parent.addChild(std::move(obj));
    return ref;
  }

};

}
