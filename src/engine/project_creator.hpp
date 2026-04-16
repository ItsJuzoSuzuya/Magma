#pragma once
#include "engine/components/camera.hpp"
#include "engine/components/mesh.hpp"
#include "engine/components/point_light.hpp"
#include "engine/components/transform.hpp"
#include "engine/gameobject.hpp"
#include "engine/project.hpp"
#include "engine/scene.hpp"
#include "engine/scene_manager.hpp"
#include <glm/trigonometric.hpp>

namespace Magma {

class ProjectCreator {
public:
  static Project initProject() {
    Project project = {};
    project.scene = SceneManager::createScene("Main");

    auto *camObj = project.scene->createGameObject("Main Camera");
    auto *camTransform = camObj->addComponent<Transform>();
    auto *cam = camObj->addComponent<Camera>();
    cam->setPerspectiveProjection(glm::radians(60.0f), 16.0f / 9.0f, 0.1f, 100.0f);
    cam->setOwnerTransform(camTransform);
    project.camera = camObj;

    auto *obj = project.scene->createGameObject("Test Object");

    Transform *transform = obj->addComponent<Transform>();
    transform->position = {0.f, 0.f, 1.f};
    transform->scale = {0.1f, 0.1f, 0.1f};

    auto *lightObj = project.scene->createGameObject("Light Object");
    lightObj->addComponent<PointLight>();

    if (!obj->addComponent<Mesh>()->load("assets/cube/scene.gltf"))
      throw std::runtime_error("Failed to load model!");

    return project;
  }
};

} // namespace Magma
