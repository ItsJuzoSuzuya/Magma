cmake_minimum_required(VERSION 4.3.1)

set(CMAKE_CXX_COMPILER clang++)

project(Magma LANGUAGES CXX)

# ------------------------
# Compiler & standard
# ------------------------
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(MAGMA_WITH_EDITOR "Build with editor & ImGui tooling" YES)

# ------------------------
# Static C++ runtime
# ------------------------
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libstdc++ -static-libgcc")

# ------------------------
# External libraries
# ------------------------
find_package(Vulkan REQUIRED)
find_package(X11 REQUIRED)

# ------------------------
# GLFW (built statically from source)
# ------------------------
include(FetchContent)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
)
FetchContent_MakeAvailable(glfw)

set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/src/external/imgui)
set(EXTERNAL_DIR ${CMAKE_SOURCE_DIR}/src/external)
include_directories(${IMGUI_DIR} ${EXTERNAL_DIR} ${CMAKE_SOURCE_DIR}/src)

# ------------------------
# Gather sources automatically
# ------------------------
file(GLOB_RECURSE CORE_SOURCES src/core/*.cpp)
file(GLOB_RECURSE ENGINE_SOURCES src/engine/*.cpp)

# ------------------------
# ImGui sources (editor only)
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
# Executable
# ------------------------
add_executable(${PROJECT_NAME}
  main.cpp
  ${CORE_SOURCES}
  ${ENGINE_SOURCES}
  ${IMGUI_SOURCES}
)

target_compile_features(${PROJECT_NAME} PUBLIC cxx_std_23)

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
