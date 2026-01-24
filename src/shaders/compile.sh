#!/usr/bin/env bash

glslc src/shaders/shader.vert -o src/shaders/shader.vert.spv 
glslc src/shaders/shader.frag -o src/shaders/shader.frag.spv
glslc src/shaders/editor.vert -o src/shaders/editor.vert.spv
glslc src/shaders/editor.frag -o src/shaders/editor.frag.spv
glslc src/shaders/imgui.frag -o src/shaders/imgui.frag.spv
