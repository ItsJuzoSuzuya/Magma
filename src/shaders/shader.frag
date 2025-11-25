#version 460

layout(location = 0) flat in uint inObjectID;

layout(location = 0) out vec4 fragColor;
#if defined(MAGMA_WITH_EDITOR)
layout(location = 1) out uint fragObjectID;
#endif

void main() {
    fragColor = vec4(1.0, 0.0, 0.0, 1.0); // Red color
    #if defined(MAGMA_WITH_EDITOR)
    fragObjectID = inObjectID;
    #endif
}
