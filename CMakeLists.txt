cmake_minimum_required(VERSION 4.3.0)

project(Magma LANGUAGES CXX)

# ------------------------
# Compiler & standard
# ------------------------
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
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
file(GLOB_RECURSE CORE_MODULES src/core/*.ixx src/core/*.cpp)
file(GLOB_RECURSE ENGINE_MODULES src/engine/*.ixx src/engine/*.cpp)
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
# Executable with modules
# ------------------------
add_executable(${PROJECT_NAME} main.cpp ${IMGUI_SOURCES})

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

# Add all module interface/source units directly to executable
target_sources(${PROJECT_NAME} PRIVATE
    FILE_SET CXX_MODULES
    FILES
        ${CORE_MODULES}
        ${ENGINE_MODULES}
        ${COMPONENTS_MODULES}
        ${WIDGETS_MODULES}
        ${RENDER_MODULES}
        ${SYSTEMS_MODULES}
)

# Link external libraries
target_link_libraries(${PROJECT_NAME} PRIVATE glfw ${X11_LIBRARIES} Vulkan::Vulkan)

# ------------------------
# Editor support
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
