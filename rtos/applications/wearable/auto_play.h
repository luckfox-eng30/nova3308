/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __AUTO_PLAY__
#define __AUTO_PLAY__

#include <rtthread.h>

rt_err_t auto_play_init(void *arg);
rt_err_t auto_play_next(void *arg);

#endif
