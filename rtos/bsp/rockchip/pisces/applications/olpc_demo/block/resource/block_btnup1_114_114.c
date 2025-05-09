/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_btnup1_114_114[25UL] =
{
    0xFF, 0x46, 0x86, 0x4F, 0x61, 0x62, 0x7F, 0xF2, 0x32, 0x80, 0x91, 0x31, 0xEA, 0xEE, 0xEF, 0xFF, 0x7F, 0xFF, 0x7F, 0xFE, 0xC2, 0x6B, 0xEB, 0xFF, 0xAC
};

image_info_t block_btnup1_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 142,
    .y = 13,
    .w = 114,
    .h = 114,
    .size = 25UL,
    .data = block_btnup1_114_114,
};

#endif
