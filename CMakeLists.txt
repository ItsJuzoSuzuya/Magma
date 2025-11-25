cmake_minimum_required(VERSION 3.10)
project(Magma C CXX)

option(MAGMA_WITH_EDITOR "Build editor & ImGui tooling" OFF)

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

if(MAGMA_WITH_EDITOR)
  set(IMGUI_SOURCES
      ${IMGUI_DIR}/imgui.cpp
      ${IMGUI_DIR}/imgui_draw.cpp
      ${IMGUI_DIR}/imgui_tables.cpp
      ${IMGUI_DIR}/imgui_widgets.cpp
      ${IMGUI_DIR}/imgui_impl_glfw.cpp
      ${IMGUI_DIR}/imgui_impl_vulkan.cpp
  )
endif()

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
    # components
    src/engine/components/transform.cpp
    src/engine/components/mesh.cpp
    src/engine/components/camera.cpp
    # renderers
    src/engine/render/offscreen_renderer.cpp
    src/engine/render/offscreen_target.cpp
    src/engine/render/swapchain_target.cpp
)

if (MAGMA_WITH_EDITOR)
  list(APPEND SOURCES
    src/engine/render/imgui_renderer.cpp
    src/engine/widgets/game_editor.cpp
    src/engine/widgets/dock_layout.cpp
    src/engine/editor_camera.cpp
    src/engine/widgets/inspector.cpp
    src/engine/widgets/inspector_menu.cpp
    src/engine/widgets/scene_tree.cpp
    src/engine/widgets/scene_menu.cpp
    src/engine/widgets/game_view.cpp
    src/engine/widgets/runtime_control.cpp
  )
  message(STATUS "Building with editor support")
else()
  message(STATUS "Building without editor support")
endif()

# Build executable
add_executable(${PROJECT_NAME} ${SOURCES} ${IMGUI_SOURCES})
if(MAGMA_WITH_EDITOR)
  #target_compile_definitions(${PROJECT_NAME} PRIVATE MAGMA_WITH_EDITOR)
endif()

find_package(X11 REQUIRED)

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE Vulkan::Vulkan glfw ${X11_LIBRARIES})
