#version 460

// In 
layout(location = 0) in vec3 inPosition;
layout(push_constant) uniform PushConstants {
    mat4 model;
    uint objectID;
} push;
layout(binding = 0, std140) uniform CameraUBO {
    mat4 projView;
} ubo;

// Out
#if defined(MAGMA_WITH_EDITOR)
layout(location = 0) flat out uint fragObjectID;
#endif

void main() {
  gl_Position = ubo.projView * push.model * vec4(inPosition, 1.0);
  #if defined(MAGMA_WITH_EDITOR)
  fragObjectID = push.objectID;
  #endif
}
