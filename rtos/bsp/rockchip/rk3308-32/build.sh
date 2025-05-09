#!/bin/bash

export LC_ALL=C.UTF-8
export LANG=C.UTF-8

CUR_DIR=$(pwd)
IMAGE=$(pwd)/Image

usage() {
    echo "usage: ./build.sh <cpu_id 0~3 or all>"
}

CPU1_SRAM_BASE=0xfff88000
CPU1_SRAM_SIZE=0x00008000

CPU2_SRAM_BASE=0xfff90000
CPU2_SRAM_SIZE=0x00010000

CPU3_SRAM_BASE=0xfffa0000
CPU3_SRAM_SIZE=0x00010000

CPU0_SRAM_BASE=0xfffb0000
CPU0_SRAM_SIZE=0x00010000

CPU1_MEM_BASE=0x00800000
CPU1_MEM_SIZE=0x00a00000

CPU2_MEM_BASE=0x01200000
CPU2_MEM_SIZE=0x00a00000

CPU3_MEM_BASE=0x01c00000
CPU3_MEM_SIZE=0x00a00000

CPU0_MEM_BASE=0x02600000
CPU0_MEM_SIZE=0x00900000

SHMEM_BASE=0x02f00000
SHMEM_SIZE=0x00100000

SHRPMSG_SIZE=0x00080000
SHRAMFS_SIZE=0x00020000
SHLOG0_SIZE=0x00001000
SHLOG1_SIZE=0x00001000
SHLOG2_SIZE=0x00001000
SHLOG3_SIZE=0x00001000

make_rtt() {
    export RTT_PRMEM_BASE=$(eval echo \$CPU$1_MEM_BASE)
    export RTT_PRMEM_SIZE=$(eval echo \$CPU$1_MEM_SIZE)
    export RTT_SRAM_BASE=$(eval echo \$CPU$1_SRAM_BASE)
    export RTT_SRAM_SIZE=$(eval echo \$CPU$1_SRAM_SIZE)
    export RTT_SHMEM_BASE=$SHMEM_BASE
    export RTT_SHMEM_SIZE=$SHMEM_SIZE

    export RTT_SHRPMSG_SIZE=$SHRPMSG_SIZE
    export RTT_SHRAMFS_SIZE=$SHRAMFS_SIZE
    export RTT_SHLOG0_SIZE=$SHLOG0_SIZE
    export RTT_SHLOG1_SIZE=$SHLOG1_SIZE
    export RTT_SHLOG2_SIZE=$SHLOG2_SIZE
    export RTT_SHLOG3_SIZE=$SHLOG3_SIZE

    export CUR_CPU=$1
    scons -c
    rm -rf $CUR_DIR/gcc_arm.ld $IMAGE/amp.img $IMAGE/rtt$1.elf $IMAGE/rtt$1.bin
    scons -j8
    cp $CUR_DIR/rtthread.elf $IMAGE/rtt$1.elf
    mv $CUR_DIR/rtthread.bin $IMAGE/rtt$1.bin
}

case $1 in
    0|1|2|3) make_rtt $1 ;;
    all) make_rtt 0; make_rtt 1; make_rtt 2; make_rtt 3 ;;
    *) usage; exit ;;
esac

#./mkimage.sh
