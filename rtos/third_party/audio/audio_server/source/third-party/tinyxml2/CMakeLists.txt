cmake_minimum_required( VERSION 2.8.8 )

set(SOURCE_FILES_TINYXML2
    tinyxml2.cpp
)

add_library(${LIB_STATIC_TINYXML2} STATIC ${SOURCE_FILES_TINYXML2})
add_definitions(-std=c++11)
set_target_properties(${LIB_STATIC_TINYXML2} PROPERTIES FOLDER "third-party/tinyxml2")

message(STATUS "cmake version ${CMAKE_VERSION} [@@]config library: third-party/TinyXML2")
