/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_MUSIC__
#define __APP_MUSIC__
#include <rtthread.h>

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

rt_err_t app_music_init(void *param);
rt_err_t app_music_design(void *param);
void app_music_name_design(char *name);
void app_music_design_update(void);
rt_err_t app_music_picture_rotate(void *param);
void app_music_page_leave(void);
void app_music_touch(struct rt_touch_data *point);

#endif
