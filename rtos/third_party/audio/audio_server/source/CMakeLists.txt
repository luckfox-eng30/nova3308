cmake_minimum_required(VERSION 3.0.0)
project(rkaudio)

ADD_DEFINITIONS( -D__LINUX__)

SET (LIBRARY_OUTPUT_PATH ${PROJECT_BINARY_DIR}/lib)

set(RKAUDIO_COMMON_SOURCE common/id3v2.c
                          common/audio_misc.c
                          common/audio_queue.c
                          common/audio_stream.c
                          common/pcmout.c
                          common/platform_linux.c
                          common/capture_impl_alsa.c
                          common/playback_impl_alsa.c)

include_directories(./)
include_directories(./common)
include_directories(third-party/prebuilt/headers/alsa-lib/include)


if(RKAUDIO_RECORDER)
ADD_DEFINITIONS( -DAUDIO_ENABLE_RECORDER)
aux_source_directory(recorder RKAUDIO_RECORDER_SOURCE)
include_directories(./recorder)
endif()

if(RKAUDIO_PLAYER)
ADD_DEFINITIONS( -DAUDIO_ENABLE_PLAYER)
aux_source_directory(player RKAUDIO_PLAYER_SOURCE)
include_directories(./player)
endif()

set(RKAUDIO_SOURCE_FILE
    ${RKAUDIO_RECORDER_SOURCE}
    ${RKAUDIO_PLAYER_SOURCE}
    ${RKAUDIO_COMMON_SOURCE}
)

if(RKAUDIO_PLUGINS)
    if(RKAUDIO_PLUGIN_RESAMPLE)
        include_directories(plugins/ssrc)
        aux_source_directory(plugins/ssrc RKAUDIO_PLUGIN_RESAMPLE_SOURCE)
        set(RKAUDIO_SOURCE_FILE
            ${RKAUDIO_SOURCE_FILE}
            ${RKAUDIO_PLUGIN_RESAMPLE_SOURCE})
    endif()
endif()

if(RKAUDIO_ENDECODE)
    if(RKAUDIO_DECODER_MP3)
        ADD_DEFINITIONS( -DAUDIO_DECODER_MP3)
        include_directories(codec/decode/mp3)
        aux_source_directory(codec/decode/mp3 RKAUDIO_DECODER_MP3_SOURCE)
        set(RKAUDIO_SOURCE_FILE
            ${RKAUDIO_SOURCE_FILE}
            ${RKAUDIO_DECODER_MP3_SOURCE})
    endif()

    if(RKAUDIO_ENCODER_MP3)
        ADD_DEFINITIONS( -DAUDIO_ENCODER_MP3)
        include_directories(codec/encode/mp3 codec/encode/mp3/source)
        aux_source_directory(codec/encode/mp3 RKAUDIO_ENCODER_MP3_SOURCE)
        aux_source_directory(codec/encode/mp3/source RKAUDIO_ENCODER_ORIGINAL_MP3_SOURCE)
        set(RKAUDIO_SOURCE_FILE
            ${RKAUDIO_SOURCE_FILE}
            ${RKAUDIO_ENCODER_MP3_SOURCE}
            ${RKAUDIO_ENCODER_ORIGINAL_MP3_SOURCE})
    endif()
endif()

ADD_LIBRARY (rkaudio SHARED ${RKAUDIO_SOURCE_FILE})
link_libraries(rkaudio)
link_directories(${PROJECT_BINARY_DIR}/lib/)

ADD_LIBRARY (rkaudio_static STATIC ${RKAUDIO_SOURCE_FILE})
link_libraries(rkaudio_static)
link_directories(${PROJECT_BINARY_DIR}/lib/)

install(TARGETS rkaudio
        LIBRARY DESTINATION "lib")

install(TARGETS rkaudio_static
        LIBRARY DESTINATION "lib")

set(RKAUDIO_HEADER
      ${CMAKE_CURRENT_SOURCE_DIR}/audio_server.h
)
install(FILES ${RKAUDIO_HEADER} DESTINATION include/)

if(RKAUDIO_TEST_DEMO)
add_subdirectory(example)
endif()