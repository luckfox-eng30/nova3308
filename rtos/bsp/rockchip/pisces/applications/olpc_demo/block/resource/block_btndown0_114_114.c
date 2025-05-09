/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_BLOCK_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char block_btndown0_114_114[20UL] =
{
    0xAB, 0xCB, 0xC1, 0x80, 0x36, 0x3D, 0x82, 0x4D, 0x2C, 0xF9, 0xAF, 0xFF, 0x7F, 0xFF, 0x7F, 0xF5, 0x4A, 0x3F, 0xFF, 0xAC
};

image_info_t block_btndown0_info =
{
    .type  = IMG_TYPE_COMPRESS,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 142,
    .y = 241,
    .w = 114,
    .h = 114,
    .size = 20UL,
    .data = block_btndown0_114_114,
};

#endif
