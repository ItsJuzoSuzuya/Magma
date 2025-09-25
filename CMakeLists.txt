cmake_minimum_required(VERSION 3.10)
project(Magma C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Vulkan
find_package(Vulkan REQUIRED)

# GLFW (system-installed)
find_package(glfw3 3.3 REQUIRED)

# Flattened ImGui folder
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/external/imgui)
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/src/external)

# Include path
include_directories(${IMGUI_DIR})
include_directories(${EXTERNAL_DIR})

# ImGui sources
set(IMGUI_SOURCES
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/imgui_impl_vulkan.cpp
)

# Your main program
set(SOURCES
    main.cpp
    src/core/render_pipeline.cpp
    src/core/window.cpp
    src/core/device.cpp
    src/core/descriptors.cpp
    src/core/swapchain.cpp
    src/core/mesh.cpp
    src/core/pipeline.cpp
    src/core/renderer.cpp
    src/core/buffer.cpp
    src/engine/engine.cpp
)

# Build executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

find_package(X11 REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw ${X11_LIBRARIES})
