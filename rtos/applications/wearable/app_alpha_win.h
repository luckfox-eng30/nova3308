/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_ALPHA_WIN__
#define __APP_ALPHA_WIN__
#include <rtthread.h>

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

extern app_disp_refrsh_param_t alpha_win_refr_param;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

struct g_alpha_win_data_t
{
    rt_uint8_t *fb;
    rt_uint32_t fblen;

    rt_int16_t  mov_xoffset;
};
extern struct g_alpha_win_data_t *g_alpha_win_data;

rt_err_t alpha_win_refresh(struct rt_display_config *wincfg, void *param);

void app_alpha_win_init(void);
void app_alpha_win_show(void);
void app_alpha_win_hide(void);

#endif
