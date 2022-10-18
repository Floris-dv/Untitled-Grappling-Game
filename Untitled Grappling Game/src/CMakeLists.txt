CMAKE_MINIMUM_REQUIRED( VERSION 3.16 )

file(GLOB CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)

add_library(MainSrc OBJECT
    ${CPP_FILES}
)

target_precompile_headers(MainSrc PUBLIC pch.h)