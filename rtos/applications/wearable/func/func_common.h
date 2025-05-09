/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_FUNC__
#define __APP_FUNC__
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

struct app_func_private
{
    rt_int8_t   alpha_win;
    rt_int8_t   func_id;
};
extern struct app_page_data_t *g_func_page;
extern struct app_touch_cb_t app_func_main_touch_cb;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

void app_func_memory_init(void);
void app_func_show(void *param);
void app_func_common_exit(void);
void app_func_merge_touch_ops(struct app_touch_cb_t *ops);
void app_func_revert_touch_ops(struct app_touch_cb_t *ops);

#endif
