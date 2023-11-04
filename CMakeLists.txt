CMAKE_MINIMUM_REQUIRED( VERSION 3.20 )

project(App VERSION 0.1 LANGUAGES CXX)
include(CheckCXXCompilerFlag) 

SET(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(VENDOR "${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/vendor")

find_package( OpenGL REQUIRED )
# find_package( glslangValidator REQUIRED )

include_directories(
	${VENDOR}/spdlog/include
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

target_sources(App PRIVATE ${SOURCE_FILES} ${VENDOR}/ImGuiBuild.cpp ${VENDOR}/spdlogBuild.cpp ${VENDOR}/Vendor.cpp)

target_precompile_headers(App PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/Untitled Grappling Game/src/pch.h")

set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT App)

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    unset(COMPILER_OPT_ARCH_AVX_SUPPORTED CACHE)
    unset(COMPILER_OPT_ARCH_AVX2_SUPPORTED CACHE)
    check_cxx_compiler_flag("/arch:AVX" COMPILER_OPT_ARCH_AVX_SUPPORTED)
    check_cxx_compiler_flag("/arch:AVX2" COMPILER_OPT_ARCH_AVX2_SUPPORTED)
    if (COMPILER_OPT_ARCH_AVX2_SUPPORTED)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX2")
    elseif (COMPILER_OPT_ARCH_AVX_SUPPORTED)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:AVX")
else()
	set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} /arch:SSE")
    endif()
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_DEBUG} /GL /fp:fast /LTCG:incremental /OPT:REF")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} /W4")
else()
    target_link_libraries(App -static-libgcc -static-libstdc++)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native -Wall -Wextra -Wpedantic -Wconversion")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-language-extension-token")
endif()

if (WIN32)
    target_link_libraries(App gdi32)
endif()

target_link_libraries(App glfw glm) 
target_compile_definitions(App PRIVATE SPDLOG_COMPILED_LIB)

file(COPY "Untitled Grappling Game/resources" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

file(COPY "Untitled Grappling Game/src/Shaders" DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/src)

file(COPY "Untitled Grappling Game/Levels" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
