#! /bin/bash
#
#  Build DL_Module Shell Script file
#
#  Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
#  SPDX-License-Identifier: Apache-2.0
#

PARM=$1
RT_CONFIG=rtconfig.h
APP_ROOT=olpc_demo

if [ -z $PARM ]; then
    scons -j8 && scons --target=ua -s
    if grep -rw "^#define OLPC_DLMODULE_ENABLE" $RT_CONFIG; then
        cd ../../../applications/rtthread-apps/
        if [ `find . -name $APP_ROOT` ]; then

            export RTT_ROOT=../../
            export BSP_ROOT=../../bsp/rockchip/pisces
            RT_CONFIG=../../bsp/rockchip/pisces/rtconfig.h

            if grep -rw "^#define OLPC_APP_CLOCK_ENABLE" $RT_CONFIG; then
                scons --app=$APP_ROOT/clock
            fi

            if grep -rw "^#define OLPC_APP_BLOCK_ENABLE" $RT_CONFIG; then
                scons --app=$APP_ROOT/block
            fi

            if grep -rw "^#define OLPC_APP_EBOOK_ENABLE" $RT_CONFIG; then
                scons --app=$APP_ROOT/ebook
            fi

            if grep -rw "^#define OLPC_APP_LYRIC_ENABLE" $RT_CONFIG; then
                scons --app=$APP_ROOT/lyric
            fi

            if grep -rw "^#define OLPC_APP_NOTE_ENABLE" $RT_CONFIG; then
                scons --app=$APP_ROOT/note
            fi

            if grep -rw "^#define OLPC_APP_SNAKE_ENABLE" $RT_CONFIG; then
                scons --app=$APP_ROOT/snake
            fi

            if grep -rw "^#define OLPC_APP_XSCREEN_ENABLE" $RT_CONFIG; then
                scons --app=$APP_ROOT/xscreen
            fi

            if grep -rw "^#define OLPC_APP_JUPITER_ENABLE" $RT_CONFIG; then
                scons --app=$APP_ROOT/jupiter
            fi
        else
            echo "Error: Can not find apps root-->"$APP_ROOT
        fi
        cd -
    fi
else
    if [ $PARM = "-c" ]; then
        scons -c
        cd ../../../applications/rtthread-apps/
        rm -f $APP_ROOT/*/*.o
        rm -f $APP_ROOT/*/resource/*.o
        rm -rf $APP_ROOT/*/$APP_ROOT/
        cd -
    fi
fi
