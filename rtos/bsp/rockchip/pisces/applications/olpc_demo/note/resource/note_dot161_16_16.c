/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_NOTE_ENABLE)
#include "applications/common/image_info.h"

static const unsigned char note_dot161_16_16[32UL] =
{
    0xF8, 0x1F, 0xF0, 0x0F, 0xC0, 0x03, 0xC0, 0x03, 0x80, 0x01, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x01, 0x80, 0x01, 0xC0, 0x03, 0xC0, 0x03, 0xF0, 0x0F, 0xFC, 0x3F
};

image_info_t note_dot161_info =
{
    .type  = IMG_TYPE_RAW,
    .pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1,
    .x = 0,
    .y = 0,
    .w = 16,
    .h = 16,
    .size = 32UL,
    .data = note_dot161_16_16,
    .colorkey = COLOR_KEY_EN | 1,
};

#endif
