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
layout(location = 0) flat out uint fragObjectID;

void main() {
  gl_Position = ubo.projView * push.model * vec4(inPosition, 1.0);
  fragObjectID = push.objectID;
}
