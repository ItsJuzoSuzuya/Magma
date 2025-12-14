#version 460

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragPositionWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;

#if defined(MAGMA_WITH_EDITOR)
layout(location = 1) out uint fragObjectID;
layout(location = 3) flat in uint inObjectID; 
#endif

void main() {
  // Simple unlit shading - use normal as color for visibility
  vec3 normal = normalize(fragNormalWorld);
  vec3 baseColor = normal * 0.5 + 0.5; // Remap from [-1,1] to [0,1]
  
  outColor = vec4(baseColor, 1.0);

  #if defined(MAGMA_WITH_EDITOR)
  fragObjectID = inObjectID;
  #endif
}
