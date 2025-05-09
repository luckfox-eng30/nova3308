/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_btndown1_114_114[29UL + 1] =
{
    0xFF, 0x46, 0x86, 0x4F, 0x61, 0x62, 0x7F, 0xF5, 0x8D, 0x54, 0x9C, 0x1D, 0xA7, 0xA8, 0xEF, 0x1A, 0x52, 0x74, 0x7F, 0xFF, 0x7F, 0xFF, 0x7F, 0xFF, 0x2E, 0x9B, 0x0F, 0xFF, 0xAC
};

image_info_t block_btndown1_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 142,
    .y = 241,
    .w = 114,
    .h = 114,
    .size = 29UL,
    .data = block_btndown1_114_114,
};

#endif
