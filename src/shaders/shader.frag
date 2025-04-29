#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWorld;
layout(location = 2) in vec3 fragNormalWorld;
layout(location = 3) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 0) uniform GlobalUbo{
  mat4 projectionMatrix;
  vec4 ambientLightColor;
  vec3 lightPosition;
  vec4 lightColor;
} ubo;

void main() {
  vec3 directionToLight = ubo.lightPosition - fragPositionWorld;
  float distanceToLight = length(directionToLight);

  float lightIntensity = max(dot(normalize(fragNormalWorld), normalize(directionToLight)), 0);
  float attenuation = lightIntensity / (1.0 + 0.01 * distanceToLight + 0.0001 * distanceToLight * distanceToLight);

  vec3 lightColor = ubo.lightColor.xyz * ubo.lightColor.w;
  vec3 diffuseLight = lightColor * attenuation;

  vec3 ambientLight = ubo.ambientLightColor.xyz * ubo.ambientLightColor.w;
  outColor = vec4((ambientLight + diffuseLight), 1.0f);
}
