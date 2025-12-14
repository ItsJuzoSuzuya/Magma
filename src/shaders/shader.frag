#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;

#if defined(MAGMA_WITH_EDITOR)
layout(location = 1) out uint fragObjectID;
layout(location = 3) flat in uint inObjectID; 
#endif

struct PointLightData {
  vec4 position;
  vec4 color;
};

layout(set = 1, binding = 0, std430) readonly buffer PointLights {
  uint lightCount;
  PointLightData lights[];
};

void main() {
  vec3 diffuseLight = vec3(0.0);

  for (uint i = 0; i < lightCount; ++i) {
    PointLightData light = lights[i];

    vec3 directionToLight = light.position.xyz - fragPositionWorld;
    float distanceToLight = length(directionToLight);

    vec3 L = normalize(directionToLight);
    vec3 N = normalize(fragNormalWorld);

    float NdotL = max(dot(N, L), 0.0);

    float attenuation = 1.0 / (1.0 + 0.01 * distanceToLight + 0.0001 * distanceToLight * distanceToLight);
    diffuseLight += light.color.rgb * light.color.a * NdotL * attenuation;
  }

  outColor = vec4(diffuseLight, 1.0f);

  #if defined(MAGMA_WITH_EDITOR)
  fragObjectID = inObjectID;
  #endif
}
