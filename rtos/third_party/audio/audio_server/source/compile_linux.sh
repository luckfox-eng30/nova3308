#!/bin/bash

TOP_DIR=$(pwd)
#TOOLS_PATH=/home/sch/rv1126/linux/prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
TOOLS_PATH=/home/sch/arm-rockchip830-linux-uclibcgnueabihf/bin/arm-rockchip830-linux-uclibcgnueabihf-
FFMPEG_LIB=/home/sch/rv1126/linux/buildroot/output/rockchip_rv1126_rv1109_libs/staging/usr/lib/
FFMPEG_INCLUDE=/home/sch/rv1126/linux/buildroot/output/rockchip_rv1126_rv1109_libs/staging/usr/include
MPP_LIB=/home/sch/rv1126/linux/buildroot/output/rockchip_rv1126_rv1109_libs/staging/usr/lib/
MPP_INCLUDE=/home/sch/rv1126/linux/buildroot/output/rockchip_rv1126_rv1109_libs/staging/usr/include
RGA_INCLUDE=/home/sch/rv1126/linux/buildroot/output/rockchip_rv1126_rv1109_libs/staging/usr/include
SDK_PATH=/home/sch/otter_ipc_linux/media/rockit/rockit
THIRD_LIB_PATH=linux_arm32
CPU=ARM32
SOC=rv1106
FF_FORMAT=OFF
FF_CODEC=OFF
FF_AVUTIL=OFF
FF_RESAMPLE=OFF
MEDIA_PLAYER=OFF
UNIT_TEST=ON
usage()
{
	case $1 in
	-l|--list)
		echo "Support Boards As Follow:"
		exit 0
		;;
	-h | --help)
		echo "Usage: $0 [OPTION]..."
		echo "   \$1 \$2 \$3       Set build VERSION TARGET_PROJECT TARGET_BOARD_CONFIG"
		echo "   -h | --help    this message"
		echo "   -l | --list    list support boards and defconfigs"
		echo "   -p | --project build default project all defconfig"
		echo "   -a | --all     build all project all defconfig"
		exit 0
		;;
	-p | --project)
		echo "Build $TARGET_PROJECT"
		exit 0
		;;
	--arch32)
		echo "Build 32bit system rockit"
		TOOLS_PATH=/home/sch/rv1126/linux/prebuilts/gcc/linux-x86/arm/gcc-arm-8.3-2019.03-x86_64-arm-linux-gnueabihf/bin/arm-linux-gnueabihf-
		FFMPEG_LIB=/home/sch/rv1126/linux/buildroot/output/rockchip_rv1126_rv1109_libs/staging/usr/lib/
		FFMPEG_INCLUDE=/home/sch/rv1126/linux/buildroot/output/rockchip_rv1126_rv1109_libs/staging/usr/include
		THIRD_LIB_PATH=linux_arm32
		;;

	--arch64)
		echo "Build 64bit system rockit"
		CPU=ARM64
		TOOLS_PATH=/home/sch/gcc-arm-10.3-2021.07-x86_64-aarch64-none-linux-gnu/bin/aarch64-none-linux-gnu-
		#TOOLS_PATH=/home/sch/rk3568/buildroot/output/rockchip_rk356x_libs/host/bin/aarch64-linux-
		FFMPEG_LIB=/home/sch/rk3568/buildroot/output/rockchip_rk356x_libs/staging/usr/lib/
		FFMPEG_INCLUDE=/home/sch/rk3568/buildroot/output/rockchip_rk356x_libs/staging/usr/include
		THIRD_LIB_PATH=linux_arm64
		;;
	esac
	case $2 in
	--update_ffmpeg)
		echo "update ffmpeg"
		rm CMakeCache.txt
		rm -rf ./install
		rm -rf ./out
		rm -rf third-party/prebuilt/headers/ffmpeg-4.1.3/include/libavcodec
		rm -rf third-party/prebuilt/headers/ffmpeg-4.1.3/include/libavformat
		rm -rf third-party/prebuilt/headers/ffmpeg-4.1.3/include/libavutil
		rm -rf third-party/prebuilt/headers/ffmpeg-4.1.3/include/libswresample
		rm third-party/prebuilt/${THIRD_LIB_PATH}/libavcodec.a
		rm third-party/prebuilt/${THIRD_LIB_PATH}/libavformat.a
		rm third-party/prebuilt/${THIRD_LIB_PATH}/libavutil.a
		rm third-party/prebuilt/${THIRD_LIB_PATH}/libswresample.a
		cp -rf ${FFMPEG_INCLUDE}/libavcodec third-party/prebuilt/headers/ffmpeg-4.1.3/include/libavcodec
		cp -rf ${FFMPEG_INCLUDE}/libavformat third-party/prebuilt/headers/ffmpeg-4.1.3/include/libavformat
		cp -rf ${FFMPEG_INCLUDE}/libavutil third-party/prebuilt/headers/ffmpeg-4.1.3/include/libavutil
		cp -rf ${FFMPEG_INCLUDE}/libswresample third-party/prebuilt/headers/ffmpeg-4.1.3/include/libswresample
		cp ${FFMPEG_LIB}/libavcodec.a third-party/prebuilt/${THIRD_LIB_PATH}/libavcodec.a
		cp ${FFMPEG_LIB}/libavformat.a third-party/prebuilt/${THIRD_LIB_PATH}/libavformat.a
		cp ${FFMPEG_LIB}/libavutil.a third-party/prebuilt/${THIRD_LIB_PATH}/libavutil.a
		cp ${FFMPEG_LIB}/libswresample.a third-party/prebuilt/${THIRD_LIB_PATH}/libswresample.a
		;;

	--update_mpp)
		echo "update mpp"
		rm CMakeCache.txt
		rm -rf ./install
		rm -rf ./out
		rm -rf third-party/prebuilt/headers/mpp
		rm third-party/prebuilt/${THIRD_LIB_PATH}/librockchip_mpp.so
		cp -rf ${MPP_INCLUDE}/rockchip third-party/prebuilt/headers/mpp
		cp ${MPP_LIB}/librockchip_mpp.so third-party/prebuilt/${THIRD_LIB_PATH}/librockchip_mpp.so
		;;

	--update_rga)
		echo "update rga"
		rm CMakeCache.txt
		rm -rf ./install
		rm -rf ./out
		rm -rf third-party/prebuilt/headers/rga
		cp -rf ${MPP_INCLUDE}/rga third-party/prebuilt/headers/rga
		;;
	esac
}

function build_rkaduio(){
	echo "build rkaudio"
	cp audio_server.h.in audio_server.h
	cmake -DCMAKE_SYSTEM_NAME=Linux\
	-DCMAKE_SYSTEM_PROCESSOR=arm\
	-DCMAKE_C_COMPILER=${TOOLS_PATH}gcc\
	-DCMAKE_CXX_COMPILER=${TOOLS_PATH}g++\
	-DCMAKE_INSTALL_LIBDIR=${TOP_DIR}/install/usr\
	-DCMAKE_INSTALL_PREFIX=${TOP_DIR}/install/usr\
	-DCMAKE_BUILD_TYPE=Release\
	-DOS_LINUX=ON -DARCH=${CPU}\
	-DCMAKE_BUILD_TYPE=Release\
	-DRKAUDIO_ENDECODE=ON\
	-DRKAUDIO_DECODER_MP3=ON\
	-DRKAUDIO_PLAYER=ON\
	-DRKAUDIO_RECORDER=ON\
	-DRKAUDIO_TEST_DEMO=ON\
	-DRKAUDIO_TEST_DEMO=ON\
	-DRKAUDIO_PLUGINS=ON\
	-DRKAUDIO_PLUGIN_RESAMPLE=ON\
	CMakeLists.txt
	make -j32
	if [ $? -eq 0 ]; then
		echo "====Build success!===="
	else
		echo "====Build failed!===="
		exit 1
	fi
}

function build_copy(){
	#Copy Image
	make install
	mv ./install/usr/librkaudio_static.a ./install/usr/lib/librkaudio_static.a
	${TOOLS_PATH}strip -s ./install/usr/lib/librkaudio.so
	${TOOLS_PATH}strip --strip-debug ./install/usr/lib/librkaudio_static.a
	pushd ./install
	tar cvf ./rkaudio.tar ./usr
	popd
	echo "====install success!===="
}

usage $*
build_rkaduio
build_copy
