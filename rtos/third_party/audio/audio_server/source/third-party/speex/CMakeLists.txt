cmake_minimum_required( VERSION 2.8.8 )

include_directories(include)

set(SOURCE_FILES_SPEEX
    src/speex_resample.c
)

add_definitions(-DHAVE_CONFIG_H)
add_library(${LIB_STATIC_SPEEX} STATIC ${SOURCE_FILES_SPEEX})
set_target_properties(${LIB_STATIC_SPEEX} PROPERTIES FOLDER "third-party/speex")

