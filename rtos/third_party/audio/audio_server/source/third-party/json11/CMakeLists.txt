cmake_minimum_required( VERSION 2.8.8 )

set(SOURCE_FILES_JSON11
    json11.cpp
)

add_library(${LIB_STATIC_JSON11} STATIC ${SOURCE_FILES_JSON11})
add_definitions(-std=c++11)
set_target_properties(${LIB_STATIC_JSON11} PROPERTIES FOLDER "third-party/json11")

message(STATUS "cmake version ${CMAKE_VERSION} [@@]config library: third-party/json11")
