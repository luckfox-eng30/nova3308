/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_NOTE_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char note_dot121_12_12[24UL] =
{
    0xF0, 0xF0, 0xC0, 0x30, 0x80, 0x10, 0x80, 0x10, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x10, 0x80, 0x10, 0xC0, 0x30, 0xF0, 0xF0
};

image_info_t note_dot121_info =
{
    .type  = IMG_TYPE_RAW,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 0,
    .y = 0,
    .w = 12,
    .h = 12,
    .size = 24UL,
    .data = note_dot121_12_12,
    .colorkey = COLOR_KEY_EN | 1,
};

#endif
