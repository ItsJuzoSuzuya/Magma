#version 460

layout(location = 0) in vec3 inPosition;

layout(push_constant) uniform PushConstants {
    mat4 model;
} push;

void main() {
    gl_Position = push.model * vec4(inPosition, 1.0);
}
