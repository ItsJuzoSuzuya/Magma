#version 460

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

layout(binding = 0, std140) uniform CameraUBO {
    mat4 projView;
} ubo;

void main() {
  gl_Position = ubo.projView * push.model * vec4(inPosition, 1.0);
}
