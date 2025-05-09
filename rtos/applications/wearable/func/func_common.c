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
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct app_page_data_t *g_func_page;
static page_refrsh_request_param_t g_refr_param;

/*
 **************************************************************************************************
 *
 * touch process
 *
 **************************************************************************************************
 */
static rt_err_t app_func_move_updn_design(void *param);
static design_cb_t setting_move_updn_design = { .cb = app_func_move_updn_design, };
static rt_err_t app_func_move_updn_design(void *param)
{
    struct app_page_data_t *page = g_func_page;
    mov_design_param *tar = (mov_design_param *)param;

    page->ver_offset += TOUCH_MOVE_STEP * tar->dir;
    if ((tar->dir > 0 && (page->ver_offset) >= tar->offset) ||
            (tar->dir < 0 && (page->ver_offset) <= tar->offset))
    {
        page->ver_offset = tar->offset;
        page->ver_page = page->ver_offset / page->ver_step;
    }
    else
    {
        app_design_request(0, &setting_move_updn_design, param);
    }

    g_refr_param.page = page;
    g_refr_param.page_num = 1;
    app_refresh_request(&g_refr_param);

    return RT_EOK;
}

static rt_err_t app_func_touch_move_updn(void *param)
{
    struct app_main_data_t  *maindata = (struct app_main_data_t *)param;
    struct app_page_data_t *page = g_func_page;

    page->ver_offset = page->ver_page * page->ver_step - maindata->yoffset;

    g_refr_param.page = page;
    g_refr_param.page_num = 1;
    g_refr_param.cb = NULL;
    app_slide_refresh(&g_refr_param);

    return RT_EOK;
}

//---------------------------------------------------------------------------------
// touch move lr
//---------------------------------------------------------------------------------
static rt_err_t app_func_move_lr_design(void *param);
static design_cb_t setting_move_lr_design = { .cb = app_func_move_lr_design, };
static rt_err_t app_func_move_lr_design(void *param)
{
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *pdata = page->private;
    mov_design_param *tar = (mov_design_param *)param;

    page->hor_offset += TOUCH_MOVE_STEP * tar->dir;
    if ((tar->dir > 0 && (page->hor_offset) >= tar->offset) ||
            (tar->dir < 0 && (page->hor_offset) <= tar->offset))
    {
        page->hor_offset = tar->offset;
        if (page->hor_offset)
        {
            g_funclist_page->win_layer = WIN_MIDDLE_LAYER;
            if (pdata->alpha_win)
            {
                app_alpha_win_hide();
            }
            app_func_exit(pdata->func_id);
            app_main_touch_unregister();
            app_main_touch_register(&app_funclist_main_touch_cb);
            g_refr_param.page = g_funclist_page;
            g_refr_param.page_num = 1;
            app_update_page(g_funclist_page);
        }
        else
        {
            app_func_resume(pdata->func_id);
            g_refr_param.page = page;
            g_refr_param.page_num = 1; // + pdata->alpha_win;
        }
    }
    else
    {
        page->next = g_funclist_page;
        g_refr_param.page = page;
        g_refr_param.page_num = 2; // + pdata->alpha_win;
        g_refr_param.auto_resize = 1;
        app_design_request(0, &setting_move_lr_design, param);
    }

    app_refresh_request(&g_refr_param);

    return RT_EOK;
}

static rt_err_t app_func_touch_move_lr(void *param)
{
    struct app_main_data_t *pdata = (struct app_main_data_t *)param;
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *priv_data = page->private;

    app_func_pause(priv_data->func_id);
    page->hor_offset = -pdata->xoffset;
    if (page->hor_offset > page->vir_w)
        page->hor_offset = page->vir_w;
    if (page->hor_offset < 0)
        page->hor_offset = 0;

    page->next = g_funclist_page;
    g_refr_param.page = page;
    g_refr_param.page_num = 2;
    g_refr_param.auto_resize = 1;
    g_refr_param.cb = NULL;
    app_slide_refresh(&g_refr_param);

    return RT_EOK;
}

//---------------------------------------------------------------------------------
// touch move up
//---------------------------------------------------------------------------------
static mov_design_param touch_moveup_design_param;
rt_err_t app_func_touch_move_up(void *param)
{
    struct app_main_data_t *maindata = (struct app_main_data_t *)param;
    struct app_page_data_t *page = g_func_page;
    struct rt_touch_data *cur_p   = &maindata->cur_point[0];
    struct rt_touch_data *down_p   = &maindata->down_point[0];
    int16_t floor_ofs, ceil_ofs;

    app_slide_refresh_undo();
    if ((maindata->dir_mode == TOUCH_DIR_MODE_UPDN) && (page->h > MENU_WIN_YRES))
    {
        if (page->ver_offset < 0)
            page->ver_offset = 0;
        if (page->ver_offset > (page->h - MENU_WIN_YRES))
            page->ver_offset = page->h - MENU_WIN_YRES;

        floor_ofs = (page->ver_offset / page->ver_step + 1) * page->ver_step;
        ceil_ofs  =  page->ver_offset / page->ver_step * page->ver_step;

        if ((floor_ofs - page->ver_offset) < (page->ver_offset - ceil_ofs))
        {
            touch_moveup_design_param.dir = 1;
            touch_moveup_design_param.offset = floor_ofs;
        }
        else
        {
            touch_moveup_design_param.dir = -1;
            touch_moveup_design_param.offset = ceil_ofs;
        }
        app_design_request(0, &setting_move_updn_design, &touch_moveup_design_param);
    }
    else if (maindata->dir_mode == TOUCH_DIR_MODE_LR)
    {
        if ((down_p->x_coordinate) > (cur_p->x_coordinate))
        {
            touch_moveup_design_param.dir = 1;
            touch_moveup_design_param.offset = MENU_WIN_XRES;
        }
        else
        {
            touch_moveup_design_param.dir = -1;
            touch_moveup_design_param.offset = 0;
        }

        app_design_request(0, &setting_move_lr_design, &touch_moveup_design_param);
    }

    return RT_EOK;
}

struct app_touch_cb_t app_func_main_touch_cb =
{
    .tp_move_updn   = app_func_touch_move_updn,
    .tp_move_lr     = app_func_touch_move_lr,
    .tp_move_up     = app_func_touch_move_up,
};

void app_func_merge_touch_ops(struct app_touch_cb_t *ops)
{
    if (ops->tp_touch_down)
        app_func_main_touch_cb.tp_touch_down = ops->tp_touch_down;

    if (ops->tp_move_lr_start)
        app_func_main_touch_cb.tp_move_lr_start = ops->tp_move_lr_start;

    if (ops->tp_move_updn_start)
        app_func_main_touch_cb.tp_move_updn_start = ops->tp_move_updn_start;

    if (ops->tp_move_lr)
        app_func_main_touch_cb.tp_move_lr = ops->tp_move_lr;

    if (ops->tp_move_updn)
        app_func_main_touch_cb.tp_move_updn = ops->tp_move_updn;

    if (ops->tp_move_up)
        app_func_main_touch_cb.tp_move_up = ops->tp_move_up;

    if (ops->tp_touch_up)
        app_func_main_touch_cb.tp_touch_up = ops->tp_touch_up;
}

void app_func_revert_touch_ops(struct app_touch_cb_t *ops)
{
    if (ops->tp_touch_down && (app_func_main_touch_cb.tp_touch_down == ops->tp_touch_down))
        app_func_main_touch_cb.tp_touch_down = NULL;

    if (ops->tp_move_lr_start && (app_func_main_touch_cb.tp_move_lr_start == ops->tp_move_lr_start))
        app_func_main_touch_cb.tp_move_lr_start = NULL;

    if (ops->tp_move_updn_start && (app_func_main_touch_cb.tp_move_updn_start == ops->tp_move_updn_start))
        app_func_main_touch_cb.tp_move_updn_start = NULL;

    if (ops->tp_move_lr && (app_func_main_touch_cb.tp_move_lr == ops->tp_move_lr))
        app_func_main_touch_cb.tp_move_lr = NULL;

    if (ops->tp_move_updn && (app_func_main_touch_cb.tp_move_updn == ops->tp_move_updn))
        app_func_main_touch_cb.tp_move_updn = NULL;

    if (ops->tp_move_up && (app_func_main_touch_cb.tp_move_up == ops->tp_move_up))
        app_func_main_touch_cb.tp_move_up = NULL;

    if (ops->tp_touch_up && (app_func_main_touch_cb.tp_touch_up == ops->tp_touch_up))
        app_func_main_touch_cb.tp_touch_up = NULL;
}

/*
 **************************************************************************************************
 *
 * initial
 *
 **************************************************************************************************
 */
void app_func_show(void *param)
{
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *pdata = page->private;

    app_main_timer_cb_unregister();
    app_main_touch_unregister();
    app_main_touch_register(&app_func_main_touch_cb);

    g_funclist_page->win_layer = WIN_BOTTOM_LAYER;
    if (pdata->alpha_win)
    {
        app_alpha_win_show();
    }

    app_update_page(page);
    g_refr_param.page = page;
    g_refr_param.page_num = 1;
    app_refresh_request(&g_refr_param);
}

void app_func_common_exit(void)
{
    struct app_page_data_t *page = g_func_page;

    rt_free_psram(page->fb);
}

void app_func_common_init(void *param)
{
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *pdata = page->private;
    struct app_lvgl_label_design title;

    /* framebuffer malloc */
    page->fblen = MENU_WIN_XRES * MENU_WIN_YRES * (MENU_WIN_COLOR_DEPTH >> 3);
    page->fb    = (rt_uint8_t *)rt_malloc_psram(page->fblen);
    RT_ASSERT(page->fb != NULL);
    rt_memset((void *)page->fb, 0x0, page->fblen);

    pdata->alpha_win = 0;
    page->w = MENU_WIN_XRES;
    page->h = MENU_WIN_YRES;
    page->vir_w = MENU_WIN_XRES;
    page->ver_offset = 0;
    page->hor_offset = 0;

    title.txt = "Work in process";
    title.ping_pong = 0;
    title.font = &lv_font_montserrat_30;
    title.align = LV_LABEL_ALIGN_CENTER;
    title.fmt = page->format;
    title.img[0].width = MENU_WIN_XRES;
    title.img[0].height = lv_font_montserrat_30.line_height;
    title.img[0].stride = MENU_WIN_FB_W * (MENU_WIN_COLOR_DEPTH >> 3);
    title.img[0].pdata = page->fb + (MENU_WIN_YRES - title.img[0].height) / 2 * MENU_WIN_XRES * (MENU_WIN_COLOR_DEPTH >> 3);
    app_lv_label_design(&title);

    app_func_set_preview(APP_FUNC_COMMON, page->fb);
}

struct app_func func_common_ops =
{
    .init = app_func_common_init,
    .enter = app_func_show,
    .exit = app_func_common_exit,
};

static rt_err_t app_func_init_design(void *param)
{
    struct app_page_data_t *page;
    struct app_func_private *pdata;

    g_func_page = page = (struct app_page_data_t *)rt_malloc(sizeof(struct app_page_data_t));
    RT_ASSERT(page != RT_NULL);
    rt_memset((void *)page, 0, sizeof(struct app_page_data_t));

    pdata = (struct app_func_private *)rt_malloc(sizeof(struct app_func_private));
    RT_ASSERT(pdata != RT_NULL);
    rt_memset((void *)pdata, 0, sizeof(struct app_func_private));

    page->id = ID_NONE;
    page->win_id = APP_CLOCK_WIN_0;
    page->win_layer = WIN_MIDDLE_LAYER;
    page->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    page->private = pdata;
    page->exit_side = EXIT_SIDE_RIGHT;
    page->touch_cb = &app_func_main_touch_cb;

    app_func_set(APP_FUNC_EXERCISE, &func_motion_ops);
    app_func_set(APP_FUNC_SETTING, &func_setting_ops);
    app_func_set(APP_FUNC_COMMON, &func_common_ops);
    app_func_set(APP_FUNC_HEARTRATE, &func_heartrate_ops);

    return RT_EOK;
}
static design_cb_t  app_func_init_design_t = {.cb = app_func_init_design,};

void app_func_memory_init(void)
{
    app_design_request(0, &app_func_init_design_t, RT_NULL);
}
