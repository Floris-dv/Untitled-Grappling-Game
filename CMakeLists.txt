CMAKE_MINIMUM_REQUIRED( VERSION 3.10 )

project(GrapplingGame VERSION 0.1)

SET(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VENDOR "${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/vendor")

find_package( OpenGL REQUIRED )

include_directories(
	${VENDOR}/spdlog/include
	${VENDOR}/assimp/include
	${VENDOR}/GLFW/include
	${VENDOR}/imgui
	${VENDOR}/glm
	"${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/src"
	${VENDOR}
	${CMAKE_CURRENT_SOURCE_DIR}/Dependencies/Include
	${OPENGL_INCLUDE_DIRS}
)
add_subdirectory(${VENDOR})

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/src")

add_executable(App)

target_sources(App PRIVATE ${SOURCE_FILES} ${Vendor}/ImGuiBuild.cpp ${Vendor}/Vendor.cpp)

target_precompile_headers(App PRIVATE pch.h)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT App)

target_link_libraries(App assimp glfw glm)

file(COPY "Untitled Grappling Game/resources" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})