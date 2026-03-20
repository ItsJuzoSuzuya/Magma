# ------------------------
# CMakeLists.txt for Magma (C++23 everywhere)
# ------------------------
cmake_minimum_required(VERSION 4.3.0)

set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_FLAGS "-stdlib=libc++ -fexperimental-library")

project(Magma C CXX)

# ------------------------
# Set compiler and standard
# ------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MAGMA_WITH_EDITOR "Build editor & ImGui tooling" OFF)

# ------------------------
# External libraries
# ------------------------
find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/external/imgui)
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/src/external)
include_directories(${IMGUI_DIR} ${EXTERNAL_DIR} ${CMAKE_SOURCE_DIR}/src)

# ------------------------
# Gather module sources
# ------------------------
file(GLOB_RECURSE ENGINE_MODULES src/engine/*.ixx src/engine/*.cpp)
file(GLOB_RECURSE CORE_MODULES src/core/*.ixx src/core/*.cpp)
file(GLOB_RECURSE COMPONENTS_MODULES src/engine/components/*.ixx src/engine/components/*.cpp)
file(GLOB_RECURSE WIDGETS_MODULES src/engine/widgets/*.ixx src/engine/widgets/*.cpp)
file(GLOB_RECURSE RENDER_MODULES src/engine/render/*.ixx src/engine/render/*.cpp)
file(GLOB_RECURSE SYSTEMS_MODULES src/engine/systems/*.ixx src/engine/systems/*.cpp)

# ------------------------
# ImGui sources (optional)
# ------------------------
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

# ------------------------
# STD Module (libc++)
# ------------------------
add_library(std_module)

target_compile_features(std_module PUBLIC cxx_std_23)
target_compile_options(std_module PUBLIC
  -stdlib=libc++
  -fexperimental-library
)

target_sources(std_module
  PUBLIC
  FILE_SET CXX_MODULES
  BASE_DIRS /usr/share/libc++/v1
  FILES /usr/share/libc++/v1/std.cppm
)

# ------------------------
# Project Modules
# ------------------------
macro(add_project_module name files)
    add_library(${name})
    target_compile_features(${name} PUBLIC cxx_std_23)
    target_sources(${name}
        PUBLIC
        FILE_SET CXX_MODULES
        FILES ${files}
    )
    target_link_libraries(${name} PUBLIC std_module)
endmacro()

add_project_module(core "${CORE_MODULES}")
add_project_module(components "${COMPONENTS_MODULES}")
add_project_module(render "${RENDER_MODULES}")
add_project_module(widgets "${WIDGETS_MODULES}")
add_project_module(systems "${SYSTEMS_MODULES}")
add_project_module(engine "${ENGINE_MODULES}")

# ------------------------
# Build executable
# ------------------------
add_executable(${PROJECT_NAME} main.cpp ${IMGUI_SOURCES})
target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

# Ensure modules build before executable
add_dependencies(${PROJECT_NAME} core components render widgets systems engine)

# Link Vulkan-Hpp module + other libraries
target_link_libraries(${PROJECT_NAME} PRIVATE std_module core components render widgets systems engine glfw ${X11_LIBRARIES})

# ------------------------
# Editor support macro
# ------------------------
if(MAGMA_WITH_EDITOR)
  target_compile_definitions(${PROJECT_NAME} PRIVATE MAGMA_WITH_EDITOR)
  message(STATUS "Building with editor support")
else()
  message(STATUS "Building without editor support")
endif()

# ------------------------
# Shaders target
# ------------------------
add_custom_target(shaders
  COMMAND ${CMAKE_COMMAND} -E env MAGMA_WITH_EDITOR=$<IF:$<BOOL:${MAGMA_WITH_EDITOR}>,ON,OFF>
          ${CMAKE_COMMAND} -E chdir ${CMAKE_SOURCE_DIR} bash src/shaders/compile.sh
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Compiling Vulkan shaders (MAGMA_WITH_EDITOR=$<IF:$<BOOL:${MAGMA_WITH_EDITOR}>,ON,OFF>)"
)
add_dependencies(${PROJECT_NAME} shaders)
