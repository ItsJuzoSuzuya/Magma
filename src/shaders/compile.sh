#!/usr/bin/env bash
set -e
# Pass the editor define to glslc when building editor variant
if [ "$MAGMA_WITH_EDITOR" = "ON" ]; then
  glslc src/shaders/shader.vert -DMAGMA_WITH_EDITOR -o src/shaders/shader.vert.spv 
  glslc src/shaders/shader.frag -DMAGMA_WITH_EDITOR -o src/shaders/shader.frag.spv
else
  glslc src/shaders/shader.vert -o src/shaders/shader.vert.spv 
  glslc src/shaders/shader.frag -o src/shaders/shader.frag.spv
fi
glslc src/shaders/imgui.frag -o src/shaders/imgui.frag.spv
