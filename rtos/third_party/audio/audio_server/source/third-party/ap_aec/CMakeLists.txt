cmake_minimum_required( VERSION 2.8.8 )

install(FILES libRKAP_AEC.so DESTINATION /usr/lib)
install(FILES libRKAP_Common.so DESTINATION /usr/lib)
install(FILES para_ser_8k.bin DESTINATION /usr/bin)
install(FILES para_ser_16k.bin DESTINATION /usr/bin)

message(STATUS "cmake version ${CMAKE_VERSION} [@@]config library: third-party/ap_sec")