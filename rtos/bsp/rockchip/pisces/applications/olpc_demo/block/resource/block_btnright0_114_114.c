/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_btnright0_114_114[23UL] =
{
    0xAB, 0x8E, 0x25, 0x4F, 0xF7, 0x94, 0xD0, 0x18, 0x52, 0x87, 0xFF, 0x7F, 0xFF, 0x78, 0x81, 0xD4, 0x73, 0xFF, 0x59, 0xE1, 0x39, 0xFF, 0xAC
};

image_info_t block_btnright0_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 256,
    .y = 127,
    .w = 114,
    .h = 114,
    .size = 23UL,
    .data = block_btnright0_114_114,
};

#endif
