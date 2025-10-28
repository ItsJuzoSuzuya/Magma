cmake_minimum_required(VERSION 3.10)
project(Magma C CXX)

set(CMAKE_CXX_STANDARD 23)
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
    # core
    src/core/render_system.cpp
    src/core/window.cpp
    src/core/device.cpp
    src/core/descriptors.cpp
    src/core/swapchain.cpp
    src/core/mesh_data.cpp
    src/core/pipeline.cpp
    src/core/renderer.cpp
    src/core/buffer.cpp
    # engine
    src/engine/engine.cpp
    src/engine/scene.cpp
    src/engine/gameobject.cpp
    src/engine/camera.cpp
    # components
    src/engine/components/transform.cpp
    src/engine/components/mesh.cpp
    # widgets
    src/engine/widgets/dock_layout.cpp
    src/engine/widgets/inspector.cpp
    src/engine/widgets/inspector_menu.cpp
    src/engine/widgets/scene_tree.cpp
    src/engine/widgets/scene_menu.cpp
    src/engine/widgets/offscreen_view.cpp
    # renderers
    src/engine/render/imgui_renderer.cpp
    src/engine/render/offscreen_renderer.cpp
    src/engine/render/offscreen_target.cpp
    src/engine/render/swapchain_target.cpp
)

# Build executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})

find_package(X11 REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw ${X11_LIBRARIES})
