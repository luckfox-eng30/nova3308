/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_PLAY_H__
#define __APP_PLAY_H__

#include "app_notice.h"

#define APP_PLAY_VOL_MIN      0
#define APP_PLAY_VOL_MAX      4
#define APP_PLAY_VOL_DEFAULT  (APP_PLAY_VOL_MAX / 2)

void app_play_set_callback(void (*cb)(void));
void app_play_init(void);
void app_play_notice(int index);
void app_play_music(char *target);
void app_play_pause(void);
void app_play_stop(void);
void app_play_random(void);
void app_play_next(void);
void app_play_prev(void);
void app_music_vol_switch(void);
void app_music_mode_switch(void);

#endif
