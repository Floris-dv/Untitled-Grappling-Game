CMAKE_MINIMUM_REQUIRED( VERSION 3.10 )

# Assimp options
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build package with shared libraries." FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "Disable Assimp's export functionality." FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "If the test suite for Assimp is built in addition to the library." FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "Disable this if you want to use assimp as a submodule." FORCE)

add_subdirectory(assimp)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation" FORCE)

add_subdirectory(GLFW)

add_subdirectory(glm)