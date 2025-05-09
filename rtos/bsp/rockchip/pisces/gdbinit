# SPDX-License-Identifier: Apache-2.0
#
# Copyright (c) 2019, Fuzhou Rockchip Electronic Co., Ltd
#
# gdbinit for GDB debug with J-Link
#
# Usage:
#   1. Start J-Link GDB server.
#      JLinkGDBServer -select USB -device Cortex-M4 -endian little -if SWD -speed 4000 -noir -LocalhostOnly
#   Notes: Don't start J-Link with -ir, it will set the device to ARM mode.
#   2. Start GDB debugger with this script
#      arm-none-eabi-gdb -x gdbinit
#

target remote localhost:2331
file rtthread.elf
load
#handle SIGINT nostop print pass
# Un-comment this, if you want to use "ctrl-c" to exit to gdb command line,
# but keep device running.
#handle SIGTRAP nostop print pass
#b rt_init_thread_entry
#b drv_display.c:264

