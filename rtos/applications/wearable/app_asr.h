/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_ASR_H__
#define __APP_ASR_H__

typedef void (*asr_cb)(void);

void app_asr_set_callback(asr_cb cb);
void app_asr_start(void);
void app_asr_stop(void);
void app_asr_pause(void);
void app_asr_resume(void);
void app_asr_deinit(void);
void app_asr_init(void);

#endif
