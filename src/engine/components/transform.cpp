module;
#include <glm/ext/matrix_float4x4.hpp>
#include <glm/ext/vector_float3.hpp>

export module components:transform;
import :component;
import core;

namespace Magma {

export class Transform : public Component {
public:
  Transform(uint64_t *ownerID) : Component(ownerID) {}
  ~Transform() = default;

  glm::vec3 position{0.0f, 0.0f, 0.0f};
  glm::vec3 rotation{0.0f, 0.0f, 0.0f};
  glm::vec3 scale{1.0f, 1.0f, 1.0f};

  void onAwake() override {};
  void onUpdate() override {};

  void collectProxy(RenderProxy &proxy) override {
    TransformProxy transformProxy = {};
    transformProxy.modelMatrix = modelMatrix;
    transformProxy.objectId = ownerID;

    proxy.transform = proxy;
  }

  #if defined(MAGMA_WITH_EDITOR)
    // Inspector
    void Transform::onInspector() {
      ImGui::DragFloat3("Position", &position.x, 0.1f);
      ImGui::DragFloat3("Rotation", &rotation.x, 0.1f);
      ImGui::DragFloat3("Scale", &scale.x, 0.1f);
    }
    const char *inspectorName() const override { return "Transform"; }
    const float inspectorHeight() const override { return 110.0f; }
  #endif

  // Direction vectors
  glm::vec3 forward() const {
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    return glm::vec3{s1 * c2, -s2, c1 * c2};
  }

  glm::vec3 right() const {
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    return glm::vec3{c1 * c3 + s1 * s2 * s3, c2 * s3,
                     c1 * s2 * s3 - s1 * c3};
  }

  glm::vec3 up() const {
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    return glm::vec3{s1 * s2 * c3 - s3 * c1, c2 * c3,
                     s1 * s3 + c1 * s2 * c3};
  }

private:
  glm::mat4 mat4() const {
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    return glm::mat4{{
                         scale.x * (c1 * c3 + s1 * s2 * s3),
                         scale.x * c2 * s3,
                         scale.x * (c1 * s2 * s3 - s1 * c3),
                         0.f,
                     },
                     {
                         scale.y * (s1 * s2 * c3 - s3 * c1),
                         scale.y * c2 * c3,
                         scale.y * (s1 * s3 + c1 * s2 * c3),
                         0.f,
                     },
                     {
                         scale.z * s1 * c2,
                         scale.z * -s2,
                         scale.z * c1 * c2,
                         0.f,
                     },
                     {position.x, position.y, position.z, 1.f}};
  }

  glm::mat4 normalMatrix() const {
    const float c1 = glm::cos(rotation.y);
    const float s1 = glm::sin(rotation.y);
    const float c2 = glm::cos(rotation.x);
    const float s2 = glm::sin(rotation.x);
    const float c3 = glm::cos(rotation.z);
    const float s3 = glm::sin(rotation.z);
    const glm::vec3 invScale = 1.f / scale;

    return glm::mat4{
        {
            invScale.x * (c1 * c3 + s1 * s2 * s3),
            invScale.x * c2 * s3,
            invScale.x * (c1 * s2 * s3 - s1 * c3),
            0.f,
        },
        {
            invScale.y * (s1 * s2 * c3 - s3 * c1),
            invScale.y * c2 * c3,
            invScale.y * (s1 * s3 + c1 * s2 * c3),
            0.f,
        },
        {
            invScale.z * s1 * c2,
            invScale.z * -s2,
            invScale.z * c1 * c2,
            0.f,
        },
        {0.f, 0.f, 0.f, 1.f},
    };
  }
};

} // namespace Magma
