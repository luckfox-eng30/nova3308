cmake_minimum_required( VERSION 2.8.8 )

if (NOT OS_ANDROID)
set(SOURCE_FILES_V4L2_SUBDEV
    v4l2subdev.c
)
endif()

set(SOURCE_FILES_MEDIACTL
    ${SOURCE_FILES_V4L2_SUBDEV}
    mediactl.c
)

add_library(${LIB_STATIC_MEDIACTL} STATIC ${SOURCE_FILES_MEDIACTL})
set_target_properties(${LIB_STATIC_MEDIACTL} PROPERTIES FOLDER "third-party/mediactl")

message(STATUS "cmake version ${CMAKE_VERSION} [@@]config library: third-party/mediactl")
