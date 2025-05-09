/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __SETTING_COMMON_H__
#define __SETTING_COMMON_H__
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

struct app_setting_private
{
    rt_int8_t   setting_id;
};
extern struct app_page_data_t *g_setting_page;
extern struct app_touch_cb_t app_setting_main_touch_cb;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

void app_setting_memory_init(void);
void app_setting_show(void *param);

#endif
