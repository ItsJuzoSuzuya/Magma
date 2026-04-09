module;
#include <memory>
#include <string>
#include <glm/trigonometric.hpp>

export module engine:project_creator;
import :scene;
import :gameobject;
import :project;
import components;

namespace Magma {

export class ProjectCreator {
public:
  ProjectCreator() = default;

  static Project initProject(){
    Project project = {};
    project.scene = std::make_unique<Scene>();

    auto id = static_cast<GameObject::id_t>(1);
    auto camObj = std::make_unique<GameObject>(id, "Main Camera");
    camObj->addComponent<Transform>();
    auto *cam = camObj->addComponent<Camera>();
    cam->setPerspectiveProjection(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);

    project.camera = camObj.get();
    project.scene->addGameObject(std::move(camObj));

    return project;
  }
};

}
