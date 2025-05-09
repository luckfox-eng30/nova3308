cmake_minimum_required( VERSION 2.8.8 )

if (HAVE_TINYXML2)
    add_subdirectory(tinyxml2)
    message(STATUS "Build WITH TinyXML2(3rd)")
else()
    message(STATUS "Build NONE TinyXML2(3rd)")
endif()

if (HAVE_UCHARDET)
    add_subdirectory(uchardet)
    message(STATUS "Build WITH Uchardet(3rd)")
else()
    message(STATUS "Build NONE Uchardet(3rd)")
endif()

add_subdirectory(json11)

if (HAVE_3A)
    # add_subdirectory(ap_aec)
    message(STATUS "Build WITH ap_aec(3rd), please build external/common_algorithm first")
else()
    message(STATUS "Build NONE ap_aec(3rd)")
endif()

if (HAVE_TINYALSA)
    add_subdirectory(tinyalsa)
    message(STATUS "Build WITH tinyalsa(3rd)")
else()
    message(STATUS "Build NONE tinyalsa(3rd)")
endif()

add_subdirectory(mediactl)

if (HAVE_GTEST)
    add_subdirectory(gtest)
    message(STATUS "Build WITH google test(3rd)")
else()
    message(STATUS "Build NONE google test(3rd)")
endif()

add_subdirectory(argparse)

if(HAVE_SPEEX_RESAMPLE)
    add_subdirectory(speex)
    message(STATUS "Build WITH speex(3rd)")
else()
    message(STATUS "Build NONE speex(3rd)")
endif()
