/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_NOTE_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char note_dot081_8_8[8UL] =
{
    0xC3, 0x81, 0x00, 0x00, 0x00, 0x00, 0x81, 0xC3
};

image_info_t note_dot081_info =
{
    .type  = IMG_TYPE_RAW,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 0,
    .y = 0,
    .w = 8,
    .h = 8,
    .size = 8UL,
    .data = note_dot081_8_8,
    .colorkey = COLOR_KEY_EN | 1,
};

#endif
