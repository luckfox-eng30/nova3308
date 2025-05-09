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
struct app_page_data_t *g_preview_page = RT_NULL;
page_refrsh_request_param_t g_refr_param;

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
 * Declaration
 *
 **************************************************************************************************
 */

static rt_err_t app_preview_touch_up(void *param)
{
    struct app_page_data_t *page = g_preview_page;

    app_main_data->clock_style = page->hor_page;
    if (app_main_data->clock_style >= CLOCK_STYLE_MAX_NUM)
    {
        app_main_data->clock_style = CLOCK_STYLE_MAX_NUM;
    }
    save_app_info(app_main_data);

    main_page_design_param_t clock_param = { .buf_id = 1};
    app_main_get_time(&app_main_data->tmr_data);
    app_clock_init(NULL);
    app_clock_design(&clock_param);

    rt_free_psram(page->fb);

    app_main_touch_unregister();
    app_main_touch_register(&main_page_touch_cb);

    app_main_timer_cb_register(app_main_page_clock_update, 1000);

    g_refr_param.page = app_main_page;
    g_refr_param.page_num = 1;
    g_refr_param.auto_resize = 0;
    app_refresh_request(&g_refr_param);
    app_update_page(app_main_page);

    return RT_EOK;
}

static void slide_lr_cb(int mov_fix)
{
    struct app_page_data_t *page = g_funclist_page;
    page->hor_offset -= mov_fix;
}

static rt_err_t app_preview_touch_move_lr(void *param)
{
    struct app_main_data_t *maindata = (struct app_main_data_t *)param;
    struct app_page_data_t *page = g_preview_page;

    page->hor_offset = page->hor_page * page->hor_step - maindata->xoffset;

    if (page->hor_offset < 0)
        page->hor_offset = 0;
    if (page->hor_offset > (PREVIEW_WIN_FB_W - PREVIEW_WIN_XRES))
        page->hor_offset = PREVIEW_WIN_FB_W - PREVIEW_WIN_XRES;

    g_refr_param.page = page;
    g_refr_param.page_num = 1;
    g_refr_param.auto_resize = 0;
    g_refr_param.cb = slide_lr_cb;
    app_slide_refresh(&g_refr_param);

    return RT_EOK;
}

static rt_err_t app_preview_move_lr_design(void *param);
design_cb_t preview_move_lr_design = { .cb = app_preview_move_lr_design, };
static rt_err_t app_preview_move_lr_design(void *param)
{
    struct app_page_data_t *page = g_preview_page;
    mov_design_param *tar = (mov_design_param *)param;

    page->hor_offset += TOUCH_MOVE_STEP * tar->dir;
    if ((tar->dir > 0 && (page->hor_offset) >= tar->offset) ||
            (tar->dir < 0 && (page->hor_offset) <= tar->offset))
    {
        page->hor_offset = tar->offset;
        page->hor_page = page->hor_offset / page->hor_step;

        app_main_touch_register(&app_preview_main_touch_cb);
    }
    else
    {
        app_design_request(0, &preview_move_lr_design, param);
    }

    g_refr_param.page = page;
    g_refr_param.page_num = 1;
    g_refr_param.auto_resize = 0;
    app_refresh_request(&g_refr_param);

    return RT_EOK;
}

static mov_design_param touch_moveup_design_param;
rt_err_t app_preview_touch_move_up(void *param)
{
    struct app_main_data_t *maindata = (struct app_main_data_t *)param;
    struct app_page_data_t *page = g_preview_page;
    int16_t floor_ofs, ceil_ofs;

    if (maindata->dir_mode == TOUCH_DIR_MODE_LR)
    {
        app_slide_refresh_undo();
        app_main_touch_unregister();

        if (page->hor_offset < 0)
            page->hor_offset = 0;
        if (page->hor_offset > (PREVIEW_WIN_FB_W - PREVIEW_WIN_XRES))
            page->hor_offset = PREVIEW_WIN_FB_W - PREVIEW_WIN_XRES;

        floor_ofs = (page->hor_offset / page->hor_step + 1) * page->hor_step;
        ceil_ofs  =  page->hor_offset / page->hor_step * page->hor_step;

        if ((floor_ofs - page->hor_offset) < (page->hor_offset - ceil_ofs))
        {
            touch_moveup_design_param.dir = 1;
            touch_moveup_design_param.offset = floor_ofs;
        }
        else
        {
            touch_moveup_design_param.dir = -1;
            touch_moveup_design_param.offset = ceil_ofs;
        }

        // rt_kprintf("floor %d(%d) ceil %d(%d)\n", floor_ofs, floor_ofs - page->hor_offset, ceil_ofs, page->hor_offset - ceil_ofs);
        // rt_kprintf("from %d to %d cur %d\n", page->hor_step * page->hor_page, touch_moveup_design_param.offset, page->hor_offset);
        app_design_request(0, &preview_move_lr_design, &touch_moveup_design_param);
    }

    return RT_EOK;
}

struct app_touch_cb_t app_preview_main_touch_cb =
{
    .tp_move_lr     = app_preview_touch_move_lr,
    .tp_move_up     = app_preview_touch_move_up,
    .tp_touch_up    = app_preview_touch_up,
};

static rt_err_t app_preview_enter_do(void *param)
{
    struct app_page_data_t *page = g_preview_page;

    /* framebuffer malloc */
    page->fblen = PREVIEW_WIN_FB_W * PREVIEW_WIN_FB_H * (PREVIEW_WIN_COLOR_DEPTH >> 3);

    page->fb = (rt_uint8_t *)rt_malloc_psram(page->fblen);
    RT_ASSERT(page->fb != RT_NULL);

    img_load_info_t img_page_bkg = { PREVIEW_WIN_FB_W, PREVIEW_WIN_FB_H, USERDATA_PATH"preview.dta"};
    app_load_img(&img_page_bkg, (rt_uint8_t *)page->fb, PREVIEW_WIN_FB_W, PREVIEW_WIN_FB_H, 0, PREVIEW_WIN_COLOR_DEPTH >> 3);

    page->hor_page = app_main_data->clock_style;
    page->hor_offset = page->hor_page * page->hor_step;

    app_main_touch_skip(RT_TOUCH_EVENT_UP);
    app_main_touch_register(&app_preview_main_touch_cb);

    g_refr_param.page = page;
    g_refr_param.page_num = 1;
    g_refr_param.auto_resize = 0;
    app_refresh_request(&g_refr_param);
    app_update_page(page);

    return RT_EOK;
}
static design_cb_t app_preview_enter_design_t = {.cb = app_preview_enter_do,};

rt_err_t app_preview_enter(void *param)
{
    app_design_request(0, &app_preview_enter_design_t, RT_NULL);

    return RT_EOK;
}

static rt_err_t app_preview_init_design(void *param)
{
    struct app_page_data_t *page;

    g_preview_page = page = (struct app_page_data_t *)rt_malloc(sizeof(struct app_page_data_t));
    RT_ASSERT(page != RT_NULL);
    rt_memset((void *)page, 0, sizeof(struct app_page_data_t));

    page->id = ID_NONE;
    page->w = PREVIEW_WIN_XRES;
    page->h = PREVIEW_WIN_YRES;
    page->vir_w = PREVIEW_WIN_FB_W;
    page->hor_step = PREVIEW_WIN_SHOW_X + PREVIEW_GAP;
    page->win_id = APP_CLOCK_WIN_2;
    page->win_layer = WIN_BOTTOM_LAYER;
    page->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    page->touch_cb = &app_preview_main_touch_cb;
    page->ver_offset = -(DISP_YRES - PREVIEW_WIN_YRES) / 2;
    page->exit_side = EXIT_SIDE_TOP;

    return RT_EOK;
}
static design_cb_t  app_preview_init_design_t = {.cb = app_preview_init_design,};

void app_preview_init(void)
{
    app_design_request(0, &app_preview_init_design_t, RT_NULL);
}
