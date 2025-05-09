/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_CLOCK__
#define __APP_CLOCK__
#include <rtthread.h>

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

#define CLOCK_STYLE_MAX_NUM     4

rt_err_t app_clock_init(void *param);
rt_err_t app_clock_design(void *param);

#endif
