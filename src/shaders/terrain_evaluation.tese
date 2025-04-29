#version 460

layout (triangles, equal_spacing, ccw) in;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPosition;
layout(location = 2) out vec3 fragNormal;

layout(set = 0, binding = 0) uniform GlobalUbo{
  mat4 projectionMatrix;
  vec4 ambientLightColor;
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

layout(set = 0, binding = 1) uniform sampler2D heightMap;

void main() {
  float u = gl_TessCoord.x;
  float v = gl_TessCoord.y;

  vec4 p0 = gl_in[0].gl_Position;
  vec4 p1 = gl_in[1].gl_Position;
  vec4 p2 = gl_in[2].gl_Position;

  vec4 position = p0 * (1.0 - u - v) + p1 * u + p2 * v;

  fragColor = vec3(1.0, 0.0, 0.0);
  fragPosition = position.xyz;
  fragNormal = vec3(0.0, 1.0, 0.0);
  
  gl_Position = ubo.projectionMatrix * position;
}
