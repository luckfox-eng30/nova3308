/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_FUNCLIST__
#define __APP_FUNCLIST__
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

extern app_disp_refrsh_param_t app_funclist_refrsh_param;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

struct app_funclist_private
{
    uint8_t  *fb;
    uint32_t anim_fblen;
    uint8_t  *anim_fb[2];
    uint8_t  buf_id;
    struct rt_touch_data point[1];
    int pos_x;
    int pos_y;
    rt_uint32_t img_buflen;
    rt_uint8_t *img_buf;
};
extern struct app_page_data_t *g_funclist_page;
extern struct app_touch_cb_t app_funclist_main_touch_cb;

void app_funclist_init(void);
rt_err_t app_funclist_page_show_funclist(void *param);
rt_err_t app_funclist_refresh(struct rt_display_config *wincfg, void *param);

#endif
