CMAKE_MINIMUM_REQUIRED( VERSION 3.10 )

# Assimp options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build package with shared libraries." FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable Assimp's export functionality." FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "If the test suite for Assimp is built in addition to the library." FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "Disable this if you want to use assimp as a submodule." FORCE)

add_subdirectory(assimp)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation" FORCE)

add_subdirectory(GLFW)

set(IMGUI_PATH ${CMAKE_CURRENT_SOURCE_DIR}/imgui)
file(GLOB IMGUI_SOURCES ${IMGUI_PATH}/*.cpp ${IMGUI_PATH}/backends/imgui_impl_glfw.cpp ${IMGUI_PATH}/backends/imgui_impl_opengl3.cpp)
add_library(ImGui OBJECT ${IMGUI_SOURCES})

add_subdirectory(glm)

# Other library files, mostly ones that only have 1 file
add_library(Vendor OBJECT glad/glad.c stb_image.cpp)