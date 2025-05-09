/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_btnup0_114_114[19UL] =
{
    0xAB, 0xCA, 0x15, 0xA1, 0xC4, 0x8C, 0x01, 0x27, 0xBF, 0xFF, 0x7F, 0xFF, 0x76, 0xF4, 0xBD, 0xCC, 0x9F, 0xFF, 0xAC
};

image_info_t block_btnup0_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 142,
    .y = 13,
    .w = 114,
    .h = 114,
    .size = 19UL,
    .data = block_btnup0_114_114,
};

#endif
