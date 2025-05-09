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
#include "hal_base.h"
#include "image_info.h"
#include "display.h"

#if defined(RT_USING_TOUCH_DRIVERS)
#include "touch.h"
#include "touchpanel.h"
#endif

#include "app_main.h"
#include "lib_imageprocess.h"

#ifdef RT_USING_FWANALYSIS
#include "rkpart.h"
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
 * Declaration
 *
 **************************************************************************************************
 */

void app_main_page_clock_update(void);
void app_main_page_weather_update(void);
void app_main_page_music_update(void);

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */

struct app_page_data_t *app_main_page = RT_NULL;
app_disp_refrsh_param_t main_page_refrsh_param;

app_main_page_timer_cb_t main_page_timer_cb[CLOCK_HOR_PAGE_MAX] =
{
    {RT_NULL, 0},
    {app_main_page_clock_update, 1000},
    {app_main_page_weather_update, 100},
    {app_main_page_music_update, 300},
};

app_main_page_timer_cb_t main_page_leave[CLOCK_HOR_PAGE_MAX] =
{
    {RT_NULL, 0},
    {RT_NULL, 0},
    {RT_NULL, 0},
    {app_music_page_leave, 0},
};

/*
 **************************************************************************************************
 *
 * Main page design
 *
 **************************************************************************************************
 */
rt_err_t app_main_page_qrcode_init(void *param)
{
    return app_qrcode_init(param);
}

rt_err_t app_main_page_qrcode_design(void *param)
{
    return app_qrcode_design(param);
}

rt_err_t app_main_page_clock_init(void *param)
{
    return app_clock_init(param);
}

rt_err_t app_main_page_clock_design(void *param)
{
    return app_clock_design(param);
}

static design_cb_t              clock_design = {.cb = app_main_page_clock_design,};
static page_refrsh_request_param_t   clock_refresh_request_param;
static rt_uint8_t  clock_second_bk = 60;
void app_main_page_clock_update(void)
{
    app_main_get_time(&app_main_data->tmr_data);
    if (clock_second_bk == app_main_data->tmr_data->tm_sec)
    {
        return;
    }
    clock_second_bk = app_main_data->tmr_data->tm_sec;

    app_design_request(0, &clock_design, NULL);

    clock_refresh_request_param.page = app_main_page;
    clock_refresh_request_param.page_num = 1;
    app_refresh_request(&clock_refresh_request_param);
}

rt_err_t app_main_page_weather_init(void *param)
{
    return app_weather_init(param);
}

static page_refrsh_request_param_t weather_refresh_request_param;
rt_err_t app_main_page_weather_design(void *param)
{
    rt_err_t ret;

    ret = app_weather_design(param);

    if (ret == RT_EOK)
    {
        weather_refresh_request_param.page = app_main_page;
        weather_refresh_request_param.page_num = 1;
        app_refresh_request(&weather_refresh_request_param);
    }

    return ret;
}

static design_cb_t weather_design = {.cb = app_main_page_weather_design,};
void app_main_page_weather_update(void)
{
    app_design_request(0, &weather_design, NULL);
}

rt_err_t app_main_page_music_init(void *param)
{
    return app_music_init(param);
}

rt_err_t app_main_page_music_design(void *param)
{
    return app_music_design(param);
}

static design_cb_t music_design = {.cb = app_music_picture_rotate,};
void app_main_page_music_update(void)
{
    static uint32_t last_tick = 0;
    if (app_main_data->play_state != PLAYER_STATE_RUNNING ||
            ((HAL_GetTick() - last_tick) < 300))
    {
        return;
    }
    last_tick = HAL_GetTick();

    app_design_request(0, &music_design, NULL);
}

/*
 **************************************************************************************************
 *
 * Display refresh
 *
 **************************************************************************************************
 */

/*
 **************************************************************************************************
 *
 * Touch process
 *
 **************************************************************************************************
 */
//---------------------------------------------------------------------------------
// touch down & up callback
//---------------------------------------------------------------------------------
rt_err_t app_show_charging_anim(void *param)
{
    app_main_touch_skip(RT_TOUCH_EVENT_UP);
    app_charging_enable(NULL);

    return RT_EOK;
}

static void page_qrcode_touch_down(void *param)
{
    app_main_register_timeout_cb(app_show_charging_anim, NULL, 300);
}

static void page_qrcode_touch_up(void *param)
{
    app_main_unregister_timeout_cb_if_is(app_show_charging_anim);
    if (main_page_timer_cb[app_main_page->hor_page].cb)
    {
        app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                   main_page_timer_cb[app_main_page->hor_page].cycle_ms);
    }
}

static void page_clock_touch_down(void *param)
{
    app_main_register_timeout_cb(app_preview_enter, NULL, 500);
}

static void page_clock_touch_up(void *param)
{
    app_main_unregister_timeout_cb_if_is(app_preview_enter);
    if (main_page_timer_cb[app_main_page->hor_page].cb)
    {
        app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                   main_page_timer_cb[app_main_page->hor_page].cycle_ms);
    }
}

static void page_music_touch_up(void *param)
{
    struct app_main_data_t *pdata = (struct app_main_data_t *)param;
    struct rt_touch_data *cur_p = &pdata->cur_point[0];

    app_music_touch(cur_p);

    if (main_page_timer_cb[app_main_page->hor_page].cb)
    {
        app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                   main_page_timer_cb[app_main_page->hor_page].cycle_ms);
    }
}

struct app_page_touch_ops page_touch_ops[CLOCK_HOR_PAGE_MAX] =
{
    {page_qrcode_touch_down, page_qrcode_touch_up},
    {page_clock_touch_down, page_clock_touch_up},
    {NULL, NULL},
    {NULL, page_music_touch_up},
};

static rt_err_t app_main_page_touch_down(void *param)
{
    struct app_page_data_t *p_page = app_main_page;

    app_main_timer_cb_unregister();

    if (page_touch_ops[p_page->hor_page].touch_down)
    {
        page_touch_ops[p_page->hor_page].touch_down(param);
    }

    app_main_get_time(&app_main_data->tmr_data);
    app_main_page_clock_design(NULL);

    return RT_EOK;
}

static rt_err_t app_main_page_touch_up(void *param)
{
    struct app_page_data_t *p_page = app_main_page;

    if (page_touch_ops[p_page->hor_page].touch_up)
    {
        page_touch_ops[p_page->hor_page].touch_up(param);
    }
    else
    {
        if (main_page_timer_cb[p_page->hor_page].cb)
        {
            app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                       main_page_timer_cb[app_main_page->hor_page].cycle_ms);
        }
    }

    return RT_EOK;
}

//---------------------------------------------------------------------------------
// touch move lr
//---------------------------------------------------------------------------------
static rt_err_t app_main_page_move_lr_design(void *param);
static design_cb_t move_lr_touch_design = { .cb = app_main_page_move_lr_design, };
static page_refrsh_request_param_t move_lr_refr_param;
static rt_err_t app_main_page_move_lr_design(void *param)
{
    struct app_page_data_t *p_page = app_main_page;
    mov_design_param *tar = (mov_design_param *)param;
    int turn_page;

    p_page->hor_offset += TOUCH_MOVE_STEP * tar->dir;
    if ((tar->dir > 0 && (p_page->hor_offset >= tar->offset)) ||
            (tar->dir < 0 && (p_page->hor_offset <= tar->offset)))
    {
        turn_page = (tar->offset != (p_page->hor_page * p_page->hor_step));
        if (turn_page)
        {
            if (main_page_leave[p_page->hor_page].cb)
            {
                main_page_leave[p_page->hor_page].cb();
            }
            p_page->hor_page += tar->dir;
            if (p_page->hor_page < 0)
            {
                p_page->hor_page += CLOCK_HOR_PAGE_MAX;
            }
            if (p_page->hor_page >= CLOCK_HOR_PAGE_MAX)
            {
                p_page->hor_page -= CLOCK_HOR_PAGE_MAX;
            }
#if (APP_PSRAM_END_RESERVE > 0)
            g_app_info->page_index = p_page->hor_page;
#endif
        }
        p_page->hor_offset = p_page->hor_page * p_page->hor_step;

        if (main_page_timer_cb[p_page->hor_page].cb)
        {
            app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                       main_page_timer_cb[app_main_page->hor_page].cycle_ms);
        }
        app_main_touch_register(&main_page_touch_cb);
    }
    else
    {
        app_design_request(0, &move_lr_touch_design, param);
    }

    move_lr_refr_param.page = app_main_page;
    move_lr_refr_param.page_num = 1;
    app_refresh_request(&move_lr_refr_param);

    return RT_EOK;
}

static void slide_lr_cb(int mov_fix)
{
    struct app_page_data_t *page = app_main_page;
    page->hor_offset -= mov_fix;
}

static rt_err_t app_main_page_touch_move_lr(void *param)
{
    struct app_main_data_t *pdata = (struct app_main_data_t *)param;
    struct app_page_data_t *page = app_main_page;

    page->hor_offset = -pdata->xoffset;
    page->hor_offset += page->hor_page * page->hor_step;

    app_main_unregister_timeout_cb_if_is(app_show_charging_anim);
    app_main_unregister_timeout_cb_if_is(app_preview_enter);

    move_lr_refr_param.page = page;
    move_lr_refr_param.page_num = 1;
    move_lr_refr_param.cb = slide_lr_cb;
    app_slide_refresh(&move_lr_refr_param);

    return RT_EOK;
}

//---------------------------------------------------------------------------------
// touch move updn
//---------------------------------------------------------------------------------
static rt_err_t               app_main_page_move_updn_design(void *param);
static design_cb_t            move_updn_touch_design = { .cb = app_main_page_move_updn_design, };
static page_refrsh_request_param_t move_updn_design_refr_param;
static page_refrsh_request_param_t move_updn_refr_param;
static rt_err_t app_main_page_move_updn_design(void *param)
{
    struct app_main_data_t *maindata = app_main_data;
    struct app_page_data_t *p_page = app_main_page;
    struct app_main_page_private *pdata = p_page->private;
    struct app_page_data_t *t_page = p_page->top;
    struct app_page_data_t *b_page = p_page->bottom;
    struct app_page_data_t *n_page;
    mov_design_param *tar = (mov_design_param *)param;

    if (pdata->tar_page == VER_PAGE_BOTTOM)
        n_page = b_page;
    else
        n_page = t_page;

    n_page->ver_offset += TOUCH_MOVE_STEP * tar->dir;
    if (((tar->dir > 0) && (n_page->ver_offset >= tar->offset)) ||
            ((tar->dir < 0) && (n_page->ver_offset <= tar->offset)))
    {
        n_page->ver_offset = tar->offset;

        if (tar->offset != 0)
        {
            if (main_page_timer_cb[p_page->hor_page].cb)
            {
                app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                           main_page_timer_cb[app_main_page->hor_page].cycle_ms);
            }
            app_main_touch_register(&main_page_touch_cb);

            maindata->ver_page = VER_PAGE_NULL;
            move_updn_design_refr_param.page = app_main_page;
            move_updn_design_refr_param.page_num = 1;
        }
        else
        {
            if (main_page_leave[p_page->hor_page].cb)
            {
                main_page_leave[p_page->hor_page].cb();
            }
            n_page->hor_offset = 0;
            n_page->hor_page = 0;
            app_update_page(n_page);
            move_updn_design_refr_param.page = n_page;
            move_updn_design_refr_param.page_num = 1;
            maindata->ver_page = pdata->tar_page;
            if (pdata->tar_page == VER_PAGE_TOP)
            {
                app_main_touch_register(&app_funclist_main_touch_cb);
            }
            else
            {
                app_main_touch_register(&app_message_main_touch_cb);
                app_message_page_new_message(NULL);
            }
        }

        p_page->h = CLOCK_WIN_YRES;
        p_page->fb = pdata->fb;
        p_page->ver_offset = 0;

        pdata->tar_page = VER_PAGE_NULL;
        app_refresh_request(&move_updn_design_refr_param);
    }
    else
    {
        app_design_request(0, &move_updn_touch_design, param);

        app_main_page->next = n_page;
        move_updn_design_refr_param.page = app_main_page;
        move_updn_design_refr_param.page_num = 2;
        move_updn_design_refr_param.auto_resize = 1;
        app_refresh_request(&move_updn_design_refr_param);
    }

    return RT_EOK;
}

rt_err_t app_main_page_touch_move_updn_start(void *param)
{
    struct app_main_data_t *pdata = (struct app_main_data_t *)param;
    struct rt_touch_data *down_p = &pdata->down_point[0];

    if (((int32_t)down_p->y_coordinate >= CLOCK_WIN_YRES / 3) &&
            ((int32_t)down_p->y_coordinate <= CLOCK_WIN_YRES * 2 / 3))
    {
        return -RT_ERROR;
    }

    return RT_EOK;
}

static void slide_updn_cb(int mov_fix)
{
    struct app_page_data_t *page = app_main_page;
    struct app_main_page_private *pdata = page->private;
    struct app_page_data_t *n_page;

    if (pdata->tar_page == VER_PAGE_BOTTOM)
        n_page = page->bottom;
    else
        n_page = page->top;

    if (n_page != NULL)
    {
        n_page->ver_offset -= mov_fix;
        if (n_page->ver_offset < -(DISP_YRES - 2))
            n_page->ver_offset = -(DISP_YRES - 2);
        if (n_page->ver_offset > (DISP_YRES - 2))
            n_page->ver_offset = (DISP_YRES - 2);
    }
}

rt_err_t app_main_page_touch_move_updn(void *param)
{
    struct app_main_data_t *maindata = (struct app_main_data_t *)param;
    struct app_page_data_t *p_page = app_main_page;
    struct app_main_page_private *pdata = p_page->private;
    struct app_page_data_t *n_page;
    struct rt_touch_data *down_p = &maindata->down_point[0];
    struct rt_touch_data *cur_p = &maindata->cur_point[0];

    if (pdata->tar_page == VER_PAGE_NULL)
    {
        if (down_p->y_coordinate > cur_p->y_coordinate)
            pdata->tar_page = VER_PAGE_BOTTOM;
        else
            pdata->tar_page = VER_PAGE_TOP;
    }

    if (pdata->tar_page == VER_PAGE_BOTTOM)
        n_page = p_page->bottom;
    else
        n_page = p_page->top;

    if (n_page == NULL)
    {
        pdata->tar_page = VER_PAGE_NULL;
        return RT_EOK;
    }
    n_page->ver_offset = -maindata->yoffset + DISP_YRES * pdata->tar_page;
    if (n_page->ver_offset < -(DISP_YRES - 2))
        n_page->ver_offset = -(DISP_YRES - 2);
    if (n_page->ver_offset > (DISP_YRES - 2))
        n_page->ver_offset = (DISP_YRES - 2);

    app_main_unregister_timeout_cb_if_is(app_show_charging_anim);
    app_main_unregister_timeout_cb_if_is(app_preview_enter);

    p_page->next = n_page;
    move_updn_refr_param.page = p_page;
    move_updn_refr_param.page_num = 2;
    move_updn_refr_param.auto_resize = 1;
    move_updn_refr_param.cb = slide_updn_cb;
    app_slide_refresh(&move_updn_refr_param);

    return RT_EOK;
}

//---------------------------------------------------------------------------------
// touch move(touch) up
//---------------------------------------------------------------------------------
static mov_design_param   move_up_design_param;

static rt_err_t app_main_page_touch_move_up(void *param)
{
    struct app_main_data_t *maindata = (struct app_main_data_t *)param;
    struct app_main_page_private *pdata = app_main_page->private;
    struct rt_touch_data *cur_p   = &maindata->cur_point[0];
    struct rt_touch_data *down_p  = &maindata->down_point[0];
    int tar_page;

    app_main_unregister_timeout_cb_if_is(app_show_charging_anim);
    app_main_unregister_timeout_cb_if_is(app_preview_enter);
    app_slide_refresh_undo();
    app_main_touch_unregister();
    if (maindata->dir_mode == TOUCH_DIR_MODE_LR)
    {
        if (ABS((int32_t)down_p->x_coordinate - (int32_t)cur_p->x_coordinate) >= CLOCK_WIN_XRES / 3)
        {
            move_up_design_param.dir = down_p->x_coordinate > cur_p->x_coordinate ? 1 : -1;
            tar_page = app_main_page->hor_page + move_up_design_param.dir;
        }
        else
        {
            move_up_design_param.dir = down_p->x_coordinate > cur_p->x_coordinate ? -1 : 1;
            tar_page = app_main_page->hor_page;
        }
        move_up_design_param.offset = CLOCK_WIN_XRES * tar_page;
        app_design_request(1, &move_lr_touch_design, &move_up_design_param);
    }
    else if (maindata->dir_mode == TOUCH_DIR_MODE_UPDN)
    {
        app_main_timer_cb_unregister();
        if (ABS((int32_t)down_p->y_coordinate - (int32_t)cur_p->y_coordinate) >= CLOCK_WIN_YRES / 2)
        {
            move_up_design_param.dir = -pdata->tar_page;
            move_up_design_param.offset = 0;
        }
        else
        {
            move_up_design_param.dir = pdata->tar_page;
            move_up_design_param.offset = CLOCK_WIN_YRES * move_up_design_param.dir;
        }
        app_design_request(1, &move_updn_touch_design, &move_up_design_param);
    }
    else
    {
        app_main_touch_register(&main_page_touch_cb);
    }

    return RT_EOK;
}

struct app_touch_cb_t main_page_touch_cb =
{
    .tp_touch_down  = app_main_page_touch_down,
    .tp_move_lr     = app_main_page_touch_move_lr,
    .tp_move_updn   = app_main_page_touch_move_updn,
    .tp_move_up     = app_main_page_touch_move_up,
    .tp_touch_up    = app_main_page_touch_up,
    .tp_move_updn_start = app_main_page_touch_move_updn_start,
};

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
static void app_main_page_mem_init(void)
{
    struct app_page_data_t *page;
    struct app_main_page_private *pdata;

    app_main_page = page = (struct app_page_data_t *)rt_malloc(sizeof(struct app_page_data_t));
    RT_ASSERT(page != RT_NULL);
    rt_memset(page, 0, sizeof(struct app_page_data_t));

    pdata = (struct app_main_page_private *)rt_malloc(sizeof(struct app_main_page_private));
    RT_ASSERT(pdata != RT_NULL);

    page->id = ID_MAIN;
    page->fblen = CLOCK_WIN_FB_W * (CLOCK_WIN_FB_H + 1) * (CLOCK_WIN_COLOR_DEPTH >> 3);
    page->fb = (rt_uint8_t *)rt_malloc_psram(page->fblen);
    RT_ASSERT(page->fb != RT_NULL);
    rt_memset(page->fb, 0, page->fblen);
    page->w = CLOCK_WIN_XRES;
    page->h = CLOCK_WIN_YRES;
    page->vir_w = CLOCK_WIN_FB_W;
    page->win_loop = 1;
    page->hor_step = CLOCK_WIN_XRES;
    page->win_id = APP_CLOCK_WIN_2;
    page->win_layer = WIN_BOTTOM_LAYER;
    page->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    page->private = pdata;
    page->touch_cb = &main_page_touch_cb;

    pdata->fb = page->fb;
    pdata->tar_page = VER_PAGE_NULL;
}

void app_main_page_init(void)
{
    app_main_page_mem_init();

    app_main_page->hor_page = CLOCK_PAGE_FOC_ID;
    app_main_page->hor_offset = app_main_page->hor_page * app_main_page->hor_step;
    get_app_info(app_main_data);
    rt_display_backlight_set(app_main_data->bl);

    rk_imagelib_init();

    struct image_st pd;
    pd.width = CLOCK_WIN_XRES;
    pd.height = CLOCK_WIN_YRES;
    pd.stride = CLOCK_WIN_FB_W * (CLOCK_WIN_COLOR_DEPTH >> 3);
    pd.format = app_main_page->format;

    pd.pdata = app_main_page->fb + 0 * CLOCK_WIN_XRES * (CLOCK_WIN_COLOR_DEPTH >> 3);
    app_main_page_qrcode_init(&pd);
    app_main_page_qrcode_design(&pd);

    pd.pdata = app_main_page->fb + 1 * CLOCK_WIN_XRES * (CLOCK_WIN_COLOR_DEPTH >> 3);
    app_main_page_clock_init(&pd);
    app_main_page_clock_design(&pd);

    pd.pdata = app_main_page->fb + 2 * CLOCK_WIN_XRES * (CLOCK_WIN_COLOR_DEPTH >> 3);
    app_main_page_weather_init(&pd);
    app_main_page_weather_design(&pd);

    pd.pdata = app_main_page->fb + 3 * CLOCK_WIN_XRES * (CLOCK_WIN_COLOR_DEPTH >> 3);
    app_main_page_music_init(&pd);
    app_main_page_music_design(&pd);

    app_enter_page(app_main_page);

    //register callback
    if (main_page_timer_cb[app_main_page->hor_page].cb)
    {
        app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                   main_page_timer_cb[app_main_page->hor_page].cycle_ms);
    }
    app_main_touch_register(&main_page_touch_cb);

#ifdef APP_WEARABLE_ANIM_AUTO_PLAY
    app_main_register_timeout_cb(auto_play_init, NULL, 1000);
#endif
}
