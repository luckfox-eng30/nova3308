/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_CHARGING__
#define __APP_CHARGING__
#include <rtthread.h>
#include "app_main.h"

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

extern app_disp_refrsh_param_t app_charging_refrsh_param;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

struct app_charging_private
{
    rt_uint8_t  buf_id;
    rt_uint8_t *fb[2];
    rt_uint32_t fblen;
    rt_uint8_t enable;

    rt_uint8_t anim_loaded;
    rt_uint8_t *anim_dta;
    rt_uint8_t *anim_buf[CHARGING_ANIM_STEP];
    rt_uint32_t *anim_lut[CHARGING_ANIM_STEP];
    rt_uint32_t anim_buflen[CHARGING_ANIM_STEP];

    rt_uint8_t steps;
    rt_uint8_t reload_idx;
};
extern struct app_charging_data_t *g_charging_data;

rt_err_t app_charging_win_refresh(struct rt_display_config *wincfg, void *param);
rt_err_t app_charging_win_refresh_lut(struct rt_display_config *wincfg, void *param);

void app_charging_init(void);
rt_err_t app_charging_enable(void *param);

#endif
