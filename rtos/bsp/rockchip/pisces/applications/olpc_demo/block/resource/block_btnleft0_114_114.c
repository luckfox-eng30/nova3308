/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_btnleft0_114_114[23UL] =
{
    0xAB, 0x8F, 0x9F, 0x2E, 0x46, 0xDF, 0x9B, 0x8C, 0xE8, 0xFF, 0x7F, 0xFE, 0x7A, 0x94, 0xBA, 0x77, 0xFF, 0x7F, 0x96, 0xD6, 0x55, 0xFF, 0xAC,
};

image_info_t block_btnleft0_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 28,
    .y = 127,
    .w = 114,
    .h = 114,
    .size = 23UL,
    .data = block_btnleft0_114_114,
};

#endif
