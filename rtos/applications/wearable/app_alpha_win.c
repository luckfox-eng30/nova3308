/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#include <stdio.h>
#include <math.h>
#include <dfs_posix.h>

#include "drv_heap.h"
#include "drv_display.h"
#include "image_info.h"
#include "display.h"

#include <littlevgl2rtt.h>
#include <lvgl/lvgl.h>

#if defined(RT_USING_TOUCH_DRIVERS)
#include "touch.h"
#include "touchpanel.h"
#endif

#include "app_main.h"

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */

#define ENABLE_ALPHA_WIN    0

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
app_disp_refrsh_param_t alpha_win_refr_param;
#if ENABLE_ALPHA_WIN
struct g_alpha_win_data_t *g_alpha_win_data = RT_NULL;
static void *old_callback;
static void *old_param;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
/**
 * top layer refresh.
 */
rt_err_t alpha_win_refresh(struct rt_display_config *wincfg, void *param)
{
    struct rt_device_graphic_info *info = &app_main_data->disp->info;
    struct g_alpha_win_data_t *pdata = g_alpha_win_data;
    struct app_func_data_t *fdata = g_func_data;
    app_disp_refrsh_param_t *par = (app_disp_refrsh_param_t *)param;
    int offset = fdata->hor_offset > 0 ? (fdata->hor_offset > ALPHA_WIN_XRES ? ALPHA_WIN_XRES : fdata->hor_offset) : 0 ;

    wincfg->winId   = par->win_id;
    wincfg->zpos    = par->win_layer;
    wincfg->alphaEn = 1;
    wincfg->alphaMode = 1;
    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_ARGB888;
    wincfg->lut     = RT_NULL;
    wincfg->lutsize = 0;
    wincfg->fb    = pdata->fb    + offset * sizeof(rt_uint32_t);
    wincfg->fblen = pdata->fblen - offset * sizeof(rt_uint32_t);
    wincfg->xVir  = ALPHA_WIN_FB_W;
    wincfg->w     = ALPHA_WIN_XRES - offset;
    wincfg->h     = ALPHA_WIN_YRES;
    wincfg->x     = ALPHA_REGION_X + ((info->width  - ALPHA_WIN_XRES) / 2);
    wincfg->y     = ALPHA_REGION_Y + ((info->height - ALPHA_WIN_YRES) / 2);
    wincfg->ylast = wincfg->y;

    RT_ASSERT(((wincfg->w * ALPHA_WIN_COLOR_DEPTH) % 32) == 0);

    RT_ASSERT((wincfg->x + wincfg->w) <= info->width);
    RT_ASSERT((wincfg->y + wincfg->h) <= info->height);

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

/**
 * draw needle.
 */
static rt_err_t app_alpha_win_init_design(void *param)
{
    struct g_alpha_win_data_t *pdata;
    int fd;

    g_alpha_win_data = pdata = (struct g_alpha_win_data_t *)rt_malloc(sizeof(struct g_alpha_win_data_t));
    RT_ASSERT(pdata != RT_NULL);
    rt_memset((void *)pdata, 0, sizeof(struct g_alpha_win_data_t));

    /* framebuffer malloc */
    pdata->fblen = ALPHA_WIN_FB_W * ALPHA_WIN_FB_H * ALPHA_WIN_COLOR_DEPTH / 8;
    pdata->fb   = (rt_uint8_t *)rt_malloc_large(pdata->fblen);
    RT_ASSERT(pdata->fb != RT_NULL);

    fd = open(USERDATA_PATH"img_alpha.dta", O_RDONLY, 0);
    read(fd, pdata->fb, pdata->fblen);
    close(fd);

    alpha_win_refr_param.win_id = APP_CLOCK_WIN_0;
    alpha_win_refr_param.win_layer = WIN_TOP_LAYER;

    return RT_EOK;
}
static design_cb_t  alpha_win_init_design_t = {.cb = app_alpha_win_init_design,};
#endif

void app_alpha_win_show(void)
{
#if ENABLE_ALPHA_WIN
    app_get_refresh_callback(alpha_win_refr_param.win_id, &old_callback, &old_param);
    app_refresh_register(alpha_win_refr_param.win_id, alpha_win_refresh, &alpha_win_refr_param);
#endif
}

void app_alpha_win_hide(void)
{
#if ENABLE_ALPHA_WIN
    app_refresh_register(alpha_win_refr_param.win_id, old_callback, old_param);
#endif
}

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
/**
 * App clock fast init.
 */
void app_alpha_win_init(void)
{
#if ENABLE_ALPHA_WIN
    app_design_request(0, &alpha_win_init_design_t, RT_NULL);
#endif
}
