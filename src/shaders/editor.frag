#version 460

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec3 fragPositionWorld;
layout(location = 2) in vec3 fragNormalWorld;

layout(location = 0) out vec4 outColor;

layout(location = 1) out uint fragObjectID;
layout(location = 3) flat in uint inObjectID; 

void main() {
  outColor = fragColor;

  fragObjectID = inObjectID;
}
