CMAKE_MINIMUM_REQUIRED( VERSION 3.20 )

project(GrapplingGame VERSION 0.1)

SET(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(VENDOR "${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/vendor")

find_package( OpenGL REQUIRED )
find_package( glslangValidator REQUIRED )

include_directories(
	${VENDOR}/spdlog/include
	${VENDOR}/assimp/include
	${VENDOR}/GLFW/include
	${VENDOR}/imgui
	${VENDOR}/glm
	"${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/src"
	${VENDOR}
	${OPENGL_INCLUDE_DIRS}
)
add_subdirectory(${VENDOR})

add_subdirectory("${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/src")

add_executable(App)

target_sources(App PRIVATE ${SOURCE_FILES} ${VENDOR}/ImGuiBuild.cpp ${VENDOR}/Vendor.cpp)

target_precompile_headers(App PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/src/pch.h)

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT App)

target_link_libraries(App assimp glfw glm)

file(COPY "Untitled Grappling Game/resources" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})