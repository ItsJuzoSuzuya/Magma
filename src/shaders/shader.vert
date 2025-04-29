#version 460

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inColor;
layout(location = 2) in vec3 inNormal;
layout(location = 3) in vec2 inTexCoord;

layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec3 fragPositionWorld;
layout(location = 2) out vec3 fragNormalWorld;
layout(location = 3) out vec2 fragTexCoord;

layout(set = 0, binding = 0) uniform GlobalUbo{
  mat4 projectionMatrix;
  vec3 ambientLightColor;
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

layout(push_constant) uniform Push {
  mat4 modelMatrix;
  mat4 normalMatrix;
} push;

const float ambient = 0.1;


void main() {
  vec4 worldPosition = push.modelMatrix * vec4(inPosition, 1.0);
  fragPositionWorld = worldPosition.xyz;

  fragNormalWorld = normalize(mat3(push.normalMatrix) * inNormal);

  gl_Position = ubo.projectionMatrix * worldPosition;
  fragColor = inColor + ubo.ambientLightColor * ambient;
  fragTexCoord = inTexCoord;
}
