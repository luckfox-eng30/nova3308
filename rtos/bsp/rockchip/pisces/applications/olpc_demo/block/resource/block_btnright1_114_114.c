/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_btnright1_114_114[30UL] =
{
    0xFF, 0x46, 0x86, 0x4F, 0x61, 0x62, 0x7A, 0x06, 0x14, 0xD5, 0x19, 0x70, 0xD4, 0x16, 0x96, 0x05, 0xFF, 0x7F, 0xFF, 0x7F, 0xDA, 0xE0, 0x1C, 0xE3, 0xFF, 0x69, 0x70, 0xF0, 0xFF, 0xAC
};

image_info_t block_btnright1_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 256,
    .y = 127,
    .w = 114,
    .h = 114,
    .size = 30UL,
    .data = block_btnright1_114_114,
};

#endif
