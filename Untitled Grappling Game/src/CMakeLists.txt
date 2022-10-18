CMAKE_MINIMUM_REQUIRED( VERSION 3.16 )

add_executable(App OBJECT
    AABB.cpp
    Camera.cpp
    DataBuffers.cpp
    Deferred Shading.cpp
    Framebuffers.cpp
    ImGuiBuild.cpp
    Log.cpp
    Main.cpp
    MarchingCubes.cpp
    Material.cpp
    Model.cpp
    MultiMesh.cpp
    pch.cpp
    PerlinNoise.cpp
    Renderer.cpp
    Setup.cpp
    Shader.cpp
    Shadows.cpp
    Texture.cpp
    Window.cpp
    pch.h
)

target_precompile_headers(App PUBLIC pch.h)

target_link_libraries(App assimp)
target_link_libraries(App GLFW)
target_link_libraries(App ImGui)