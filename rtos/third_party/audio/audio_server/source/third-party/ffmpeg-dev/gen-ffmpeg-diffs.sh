#! /usr/bin/env bash
FFMPEG_ROOT=`pwd`
FFMPEG_PROJ=${FFMPEG_ROOT}/FFmpeg
FFMPEG_DIFF=${FFMPEG_ROOT}/FFmpegDiff
DIFF_LIBAVUTIL=1
DIFF_LIBAVCODEC=1
DIFF_LIBAVFORMAT=1
DIFF_LIBAVFILTER=0
DIFF_LIBAVDEVICE=0
DIFF_LIBPOSTPROC=0
DIFF_LIBSWSCALE=0
DIFF_LIBSWRESAMPLE=0
DIFF_LIBAVRESAMPLE=0

cd ${FFMPEG_PROJ}
AV_DIRS=(libavutil libavcodec libavformat libavfilter libavdevice libpostproc libswscale libswresample libavresample)

#generate ffmpeg different files
function do_copy_ffmpeg_diff_files() {
    echo "copy $1 different files"
    AV_DIR=$1
    DEST_AV_DIR=${FFMPEG_DIFF}/${AV_DIR}
    DIFF_FILES=`git status ${AV_DIR} -s | busybox awk '{ print $2 }'`
    if [ -n "${DIFF_FILES}" ]; then
        echo "${AV_DIR}: ${DIFF_FILES}"
        [ ! -d ${DEST_AV_DIR} ] && mkdir ${DEST_AV_DIR}
        cp -rf ${DIFF_FILES} ${DEST_AV_DIR}
    fi
}

function gen_ffmpeg_diff_files() {
    [ ! -d ${FFMPEG_DIFF} ] && mkdir ${FFMPEG_DIFF}

    for avdir in ${AV_DIRS[@]}
    do
        LIB_AV_DIFF="DIFF_"$(echo $avdir | tr '[a-z]' '[A-Z]')
        eval LIB_AV_DIFF='${'"$LIB_AV_DIFF"'}'
        if [ $LIB_AV_DIFF -eq 1 ]; then
            do_copy_ffmpeg_diff_files $avdir
        fi
    done
    echo "generate ffmpeg diff files succeed!!!"
}

#merge ffmpeg different files into FFmpeg project
function merge_ffmpeg_diff_files() {
    if [ -d "${FFMPEG_DIFF}" ]; then
        cp -rf ${FFMPEG_DIFF}"/." ${FFMPEG_PROJ}
        echo "merge ${FFMPEG_DIFF} into ${FFMPEG_PROJ}"
        echo "merge ffmpeg diff files succeed!!!"
    else
        echo "FFmpegDiff not existed!!!"
    fi
}

function merge_into_ffmpeg_confirm() {
    read -r -p "Confirm to merge ffmpeg diffs into the FFmpeg project? [Y/n] " input

    case $input in
        [yY][eE][sS]|[yY])
            echo "Yes"
            merge_ffmpeg_diff_files
            ;;

        [nN][oO]|[nN])
            echo "No"
            ;;

        *)
            echo "Invalid input..."
            exit 1
            ;;
    esac
}

if [ "$1" = "merge" ]; then
    merge_into_ffmpeg_confirm
else
    gen_ffmpeg_diff_files
fi

cd ${FFMPEG_ROOT}