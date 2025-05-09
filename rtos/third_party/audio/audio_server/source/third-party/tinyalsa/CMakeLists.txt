cmake_minimum_required( VERSION 2.8.8 )

include_directories(include)

set(SOURCE_FILES_TINYALSA
    mixer.c
    pcm.c
)

add_library(${LIB_STATIC_TINYALSA} STATIC ${SOURCE_FILES_TINYALSA})
if (OS_ANDROID)
    add_definitions(-std=c99)
endif()
set_target_properties(${LIB_STATIC_TINYALSA} PROPERTIES FOLDER "third-party/tinyalsa")
target_link_libraries(${LIB_STATIC_TINYALSA})
message(STATUS "cmake version ${CMAKE_VERSION} [@@]config library: third-party/TINYALSA")
