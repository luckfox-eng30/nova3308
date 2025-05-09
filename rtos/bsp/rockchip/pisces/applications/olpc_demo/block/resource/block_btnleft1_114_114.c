/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_btnleft1_114_114[30UL] =
{
    0xFF, 0x46, 0x86, 0x4F, 0x61, 0x62, 0x78, 0x44, 0xAA, 0x54, 0x17, 0xF9, 0x42, 0xF3, 0x7F, 0xFF, 0x7F, 0xFE, 0x94, 0xD0, 0xCF, 0x68, 0x3F, 0xFF, 0x7A, 0x11, 0x86, 0xC5, 0xFF, 0xAC
};

image_info_t block_btnleft1_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 28,
    .y = 127,
    .w = 114,
    .h = 114,
    .size = 30UL,
    .data = block_btnleft1_114_114,
};

#endif
