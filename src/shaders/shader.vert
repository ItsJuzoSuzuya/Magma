#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec3 inColor;

struct ObjectData {
    mat4 model;
    mat4 normal;
    uint objectID;
};

layout(binding = 0, std140) uniform CameraUBO {
    mat4 projView;
} ubo;

layout(set = 1, binding = 0, std430) readonly buffer ObjectSSBO {
  ObjectData objects[];
} objectBuffer;

layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec3 fragPositionWorld;
layout(location = 2) out vec3 fragNormalWorld;

void main() {
  vec4 worldPos = objectBuffer.objects[gl_InstanceIndex].model * vec4(inPosition, 1.0);

  fragNormalWorld = normalize(mat3(objectBuffer.objects[gl_InstanceIndex].normal) * inNormal);
  fragPositionWorld = worldPos.xyz;
  fragColor = vec4(inColor, 1.f);

  gl_Position = ubo.projView * worldPos;
}
