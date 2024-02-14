#version 460
layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec3 fragNormal;

layout (location = 0) out vec4 outColor;


layout(push_constant) uniform Push {
  mat4 transform;
  vec3 color;
} push;

void main(){
  float diffuse = clamp(dot(fragNormal, vec3(1.0, -1.0, 0.0)), 0, 1);
  
  outColor = vec4(diffuse * fragColor, 1.0); 
}
