/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_PREVIEW__
#define __APP_PREVIEW__
#include <rtthread.h>

#if defined(RT_USING_TOUCH_DRIVERS)
#include "touch.h"
#include "touchpanel.h"
#endif

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */

/*
 **************************************************************************************************
 *
 * Struct & data define
 *
 **************************************************************************************************
 */

extern struct app_page_data_t *g_preview_page;
extern struct app_touch_cb_t app_preview_main_touch_cb;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

void app_preview_init(void);
rt_err_t app_preview_enter(void *arg);

#endif
