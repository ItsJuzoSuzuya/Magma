module;
#include <algorithm>
#include <string>

module systems:project_creator;
import engine:scene;

namespace Magma {

export class ProjectCreator {
public:
  ProjectCreator() = default;

  static Project initProject(){
    // Create Path 
    Project project = {};

    project.scene = std::make_unique<Scene>(); 

    auto &camera = project.scene.createGameObject("Main Camera");
    camera.addComponent<Transform>();
    camera.addComponent<Camera>()->setPerspectiveProjection(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    Scene::setActiveCamera(&camera);
    return project;
  }
};

}
