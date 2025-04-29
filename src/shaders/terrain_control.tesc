#version 460

layout(vertices = 3) out;

layout(location = 0) in vec3 inColor[];
layout(location = 1) in vec3 inPosition[];
layout(location = 2) in vec3 inNormal[];

void main() {
  gl_out[gl_InvocationID].gl_Position = gl_in[gl_InvocationID].gl_Position;

  gl_TessLevelOuter[0] = 10.0;
  gl_TessLevelOuter[1] = 10.0;
  gl_TessLevelOuter[2] = 10.0;

  gl_TessLevelInner[0] = 10.0;
  gl_TessLevelInner[1] = 10.0;
}
