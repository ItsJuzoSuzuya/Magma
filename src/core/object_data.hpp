#pragma once
#include <cstdint>
#include <glm/ext/matrix_float4x4.hpp>

struct ObjectData {
  glm::mat4 modelMatrix{1.f};
  glm::mat4 normalMatrix{1.f};
  uint32_t objectID;
};

struct ObjectStorageSSBO {
  ObjectData objects[1024] = {};
};
