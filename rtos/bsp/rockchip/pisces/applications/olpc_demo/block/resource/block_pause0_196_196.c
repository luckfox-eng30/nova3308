/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_pause0_196_196[28UL] =
{
    0xAB, 0xE7, 0x1C, 0x69, 0x24, 0x2A, 0x7F, 0x0C, 0xCA, 0x99, 0xFB, 0xAA, 0x27, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0x40, 0x16, 0xC9, 0xB4, 0x35, 0xFF, 0xAC
};

image_info_t block_pause0_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 832,
    .y = 86,
    .w = 196,
    .h = 196,
    .size = 28UL,
    .data = block_pause0_196_196,
};

#endif
