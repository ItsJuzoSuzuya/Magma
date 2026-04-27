#!/usr/bin/env bash

glslc --target-env=vulkan1.3 src/shaders/shader.vert -o src/shaders/shader.vert.spv
glslc --target-env=vulkan1.3 src/shaders/shader.frag -o src/shaders/shader.frag.spv
glslc --target-env=vulkan1.3 src/shaders/editor.vert -o src/shaders/editor.vert.spv
glslc --target-env=vulkan1.3 src/shaders/editor.frag -o src/shaders/editor.frag.spv
glslc --target-env=vulkan1.3 src/shaders/imgui.frag -o src/shaders/imgui.frag.spv
