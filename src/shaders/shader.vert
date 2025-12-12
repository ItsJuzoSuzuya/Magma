#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;

layout(push_constant) uniform PushConstants {
    mat4 model;
    uint objectID;
} push;
layout(binding = 0, std140) uniform CameraUBO {
    mat4 projView;
} ubo;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPositionWorld;
layout(location = 2) out vec3 fragNormalWorld;

#if defined(MAGMA_WITH_EDITOR)
layout(location = 3) flat out uint fragObjectID;
#endif

void main() {
  vec4 worldPos = push.model * vec4(inPosition, 1.0);
  fragPositionWorld = worldPos.xyz;
  fragNormalWorld = mat3(push.model) * inNormal;

  gl_Position = ubo.projView * worldPos;

  #if defined(MAGMA_WITH_EDITOR)
    fragObjectID = push.objectID;
  #endif
}
