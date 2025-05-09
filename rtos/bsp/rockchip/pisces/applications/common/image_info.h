/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  ******************************************************************************
  * @file    image_info.h
  * @version V0.1
  * @brief   image info
  *
  * Change Logs:
  * Date           Author          Notes
  * 2019-07-18     Tony.zheng      first implementation
  *
  ******************************************************************************
  */

#ifndef __IMAGE_INFO_H__
#define __IMAGE_INFO_H__
#include <rtthread.h>

#ifdef RT_USING_PISCES_TOUCH
#include "drv_touch.h"
#include "olpc_touch.h"
#endif

/**
 * Image info for display
 */
typedef struct
{
#ifdef RT_USING_PISCES_TOUCH
    struct olpc_touch_item touch_item;
    enum olpc_touch_event touch_state;
    rt_err_t (*touch_callback)(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter);
    void *parameter;

    uint8_t touchable;
    uint8_t touch_id;
    uint16_t x_scr;
    uint16_t y_scr;
#endif

    uint8_t  type;
    uint8_t  pixel;

    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    uint32_t size;
    const uint8_t *data;
    uint32_t colorkey;
} image_info_t;

/**
 * image_info_t "type" define
 */
#define IMG_TYPE_RAW        0
#define IMG_TYPE_COMPRESS   1

#define COLOR_KEY_EN        (0x01UL << 24)

#endif
