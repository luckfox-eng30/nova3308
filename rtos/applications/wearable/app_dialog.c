/**
  * Copyright (c) 2021 Rockchip Electronics Co., Ltd
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

#define DIALOG_TOP_PADDING      10
#define DIALOG_SIDE_PADDING     10
#define DIALOG_BUTTON_HEIGHT    60

struct
{
    struct app_page_data_t *content;
    struct app_page_data_t *btn;
    struct app_page_data_t *par;

    enum dialog_type type;
    void (*cb)(enum dialog_cb_type type);

    void (*timer_cb)(void);
    uint32_t timer_ms;

    uint8_t en;
} g_dialog = {NULL, NULL, NULL, 0, NULL, NULL, 0, 0};
static page_refrsh_request_param_t dialog_refr_param;

int app_dialog_check(void)
{
    int page_num = 1;

    if (g_dialog.en == 1)
    {
        if (g_dialog.type == DIALOG_TEXT_OK_CANCEL ||
            g_dialog.type == DIALOG_IMG_OK_CANCEL ||
            g_dialog.type == DIALOG_TEXT_OK_ONLY ||
            g_dialog.type == DIALOG_IMG_OK_ONLY)
        {
            page_num = 2;
        }
        g_dialog.content->next = g_dialog.btn;
        dialog_refr_param.page = g_dialog.content;
        dialog_refr_param.page_num = page_num;
        dialog_refr_param.auto_resize = page_num > 1 ? 1 : 0;

        app_page_refresh(dialog_refr_param.page,
                         dialog_refr_param.page_num,
                         dialog_refr_param.auto_resize);

        return RT_TRUE;
    }

    return RT_FALSE;
}

int app_dialog_exit(enum dialog_cb_type type)
{
    struct app_page_data_t *page = g_dialog.content;
    struct app_page_data_t *page_btn = g_dialog.btn;

    if (page->fb)
    {
        rt_free_psram(page->fb);
        page->fb = NULL;
    }
    if (page_btn->fb)
    {
        rt_free_psram(page_btn->fb);
        page_btn->fb = NULL;
    }
    if (g_dialog.par)
    {
        app_enter_page(g_dialog.par);
        app_main_touch_register(g_dialog.par->touch_cb);
        g_dialog.par = NULL;
    }
    if (g_dialog.cb)
    {
        g_dialog.cb(type);
        g_dialog.cb = NULL;
    }

    app_main_timer_cb_register(g_dialog.timer_cb, g_dialog.timer_ms);
    g_dialog.timer_cb = NULL;
    g_dialog.en = 0;

    return RT_EOK;
}

static rt_err_t dialog_touch_down(void *param)
{

    return RT_EOK;
}

static rt_err_t dialog_touch_move_updn(void *param)
{

    return RT_EOK;
}

static rt_err_t dialog_touch_up(void *param)
{
    struct app_main_data_t *pdata = (struct app_main_data_t *)param;
    struct rt_touch_data *cur_p = &pdata->cur_point[0];
    enum dialog_cb_type type;

    if (g_dialog.type == DIALOG_TEXT_OK_CANCEL ||
        g_dialog.type == DIALOG_IMG_OK_CANCEL ||
        g_dialog.type == DIALOG_TEXT_OK_ONLY ||
        g_dialog.type == DIALOG_IMG_OK_ONLY)
    {
        if (cur_p->y_coordinate > (DIALOG_WIN_YRES - DIALOG_BUTTON_HEIGHT))
        {
            if ((g_dialog.type == DIALOG_TEXT_OK_CANCEL ||
                g_dialog.type == DIALOG_IMG_OK_CANCEL) &&
                cur_p->x_coordinate > (DIALOG_WIN_XRES / 2))
            {
                type = DIALOG_CALLBACK_CANCEL;
            }
            else
            {
                type = DIALOG_CALLBACK_OK;
            }
            app_dialog_exit(type);
        }
    }

    return RT_EOK;
}

struct app_touch_cb_t dialog_touch_cb =
{
    .tp_touch_down  = dialog_touch_down,
    .tp_move_updn   = dialog_touch_move_updn,
    .tp_touch_up    = dialog_touch_up,
};

extern void app_main_set_panel_bl(void *param);
int app_dialog_enter(struct app_page_data_t *p_page, struct dialog_desc *desc,
                     int refr_now, void (*cb)(enum dialog_cb_type type))
{
    struct app_page_data_t *page = g_dialog.content;
    struct app_page_data_t *page_btn = g_dialog.btn;
    int h = DIALOG_WIN_YRES;
    int page_num = 1;

    if (!g_dialog.en)
    {
        g_dialog.timer_cb = app_main_data->timer_cb;
        g_dialog.timer_ms = app_main_data->timer_cycle * 1000 / RT_TICK_PER_SECOND;
        g_dialog.par = p_page;
    }
    app_main_timer_cb_unregister();
    app_main_touch_unregister();

    if (page->fb)
    {
        rt_free_psram(page->fb);
        page->fb = NULL;
    }
    if (page_btn->fb)
    {
        rt_free_psram(page_btn->fb);
        page_btn->fb = NULL;
    }

    g_dialog.cb = cb;
    g_dialog.type = desc->type;
    if (desc->type == DIALOG_TEXT_OK_CANCEL ||
        desc->type == DIALOG_IMG_OK_CANCEL ||
        desc->type == DIALOG_TEXT_OK_ONLY ||
        desc->type == DIALOG_IMG_OK_ONLY)
    {
        page_btn->fb = rt_malloc_psram(page_btn->fblen);
        RT_ASSERT(page_btn->fb != RT_NULL);
        h -= DIALOG_BUTTON_HEIGHT;
        page_num = 2;
        app_main_touch_register(&dialog_touch_cb);
    }

    if ((desc->type == DIALOG_TEXT_OK_CANCEL ||
        desc->type == DIALOG_TEXT_OK_ONLY ||
        desc->type == DIALOG_TEXT_NO_CHECK) &&
        desc->text)
    {
        struct app_lvgl_label_design text;
        int strh;

        strh = strlen(desc->text) / 20 * (lv_font_montserrat_44.line_height + 5);

        page->h = h < strh ? strh : DIALOG_WIN_YRES;
        page->fblen = DIALOG_WIN_FB_W * page->h * (DIALOG_WIN_COLOR_DEPTH >> 3);
        page->fb = rt_malloc_psram(page->fblen);
        RT_ASSERT(page->fb != RT_NULL);
        memset(page->fb, 0x0, page->fblen);

        text.txt = desc->text;
        text.ping_pong = 0;
        text.font = &lv_font_montserrat_44;
        text.align = LV_LABEL_ALIGN_CENTER;
        text.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        text.img[0].width = DIALOG_WIN_XRES - 2 * DIALOG_SIDE_PADDING;
        text.img[0].height = h;
        text.img[0].stride = DIALOG_WIN_XRES * (DIALOG_WIN_COLOR_DEPTH >> 3);
        text.img[0].pdata = page->fb + DIALOG_TOP_PADDING * text.img[0].stride;
        RT_ASSERT(text.img[0].pdata != NULL);
        app_lv_label_design(&text);
    }

    if (desc->type == DIALOG_IMG_OK_CANCEL ||
        desc->type == DIALOG_IMG_OK_ONLY ||
        desc->type == DIALOG_IMG_NO_CHECK)
    {
        h = page->h = h < desc->img.h ? desc->img.h : DIALOG_WIN_YRES;
        if (page_num == 2)
            page->h += DIALOG_BUTTON_HEIGHT;
        page->fblen = DIALOG_WIN_FB_W * page->h * (DIALOG_WIN_COLOR_DEPTH >> 3);
        page->fb = rt_malloc_psram(page->fblen);
        RT_ASSERT(page->fb != RT_NULL);
        memset(page->fb, 0x0, page->fblen);

        app_load_img(&desc->img, page->fb, DIALOG_WIN_FB_W, h,
                     (DIALOG_WIN_FB_W - desc->img.w) / 2, DIALOG_WIN_COLOR_DEPTH >> 3);
    }

    if (desc->type == DIALOG_TEXT_OK_CANCEL ||
        desc->type == DIALOG_IMG_OK_CANCEL)
    {
        img_load_info_t info_ok = {DIALOG_WIN_XRES / 2, DIALOG_BUTTON_HEIGHT, ICONS_PATH"/dialog_ok.dta"};
        img_load_info_t info_cancel = {DIALOG_WIN_XRES / 2, DIALOG_BUTTON_HEIGHT, ICONS_PATH"/dialog_cancel.dta"};
        app_load_img(&info_ok, page_btn->fb,
                     DIALOG_WIN_FB_W, DIALOG_BUTTON_HEIGHT,
                     0, DIALOG_WIN_COLOR_DEPTH >> 3);
        app_load_img(&info_cancel, page_btn->fb,
                     DIALOG_WIN_FB_W, DIALOG_BUTTON_HEIGHT,
                     DIALOG_WIN_XRES / 2, DIALOG_WIN_COLOR_DEPTH >> 3);
    }

    if (desc->type == DIALOG_TEXT_OK_ONLY ||
        desc->type == DIALOG_IMG_OK_ONLY)
    {
        img_load_info_t info_ok = {DIALOG_WIN_XRES, DIALOG_BUTTON_HEIGHT, ICONS_PATH"/dialog_okonly.dta"};
        app_load_img(&info_ok, page_btn->fb,
                     DIALOG_WIN_FB_W, DIALOG_BUTTON_HEIGHT,
                     0, DIALOG_WIN_COLOR_DEPTH >> 3);
    }

    page->next = page_btn;
    dialog_refr_param.page = page;
    dialog_refr_param.page_num = page_num;
    dialog_refr_param.auto_resize = page_num > 1 ? 1 : 0;

    if (refr_now)
    {
        if (app_main_data->bl_en)
        {
            app_main_keep_screen_on();
        }
        else
        {
            app_main_set_panel_bl((void *)1);
        }

        app_page_refresh(dialog_refr_param.page,
                         dialog_refr_param.page_num,
                         dialog_refr_param.auto_resize);
    }
    else
    {
        if (app_main_data->bl_en)
        {
            app_main_keep_screen_on();
        }
        else
        {
            clock_app_mq_t mq;
            mq.cmd = MQ_BACKLIGHT_SWITCH;
            mq.param = (void *)1;
            rt_err_t ret = rt_mq_send(app_main_data->mq, &mq, sizeof(clock_app_mq_t));
            RT_ASSERT(ret != -RT_ERROR);
        }

        app_refresh_request(&dialog_refr_param);
    }
    app_update_page(page);
    g_dialog.en = 1;

    return RT_EOK;
}

void app_dialog_init(void)
{
    struct app_page_data_t *page;

    g_dialog.content = page = (struct app_page_data_t *)rt_malloc(sizeof(struct app_page_data_t));
    RT_ASSERT(page != RT_NULL);
    rt_memset((void *)page, 0, sizeof(struct app_page_data_t));

    page->id = ID_NONE;
    page->w = DIALOG_WIN_XRES;
    page->h = DIALOG_WIN_YRES;
    page->vir_w = DIALOG_WIN_FB_W;
    page->win_id = APP_CLOCK_WIN_0;
    page->win_layer = WIN_MIDDLE_LAYER;
    page->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    page->fblen = DIALOG_WIN_FB_W * DIALOG_WIN_FB_H * (DIALOG_WIN_COLOR_DEPTH >> 3);

    g_dialog.btn = page = (struct app_page_data_t *)rt_malloc(sizeof(struct app_page_data_t));
    RT_ASSERT(page != RT_NULL);
    rt_memset((void *)page, 0, sizeof(struct app_page_data_t));

    page->id = ID_NONE;
    page->w = DIALOG_WIN_XRES;
    page->h = DIALOG_BUTTON_HEIGHT;
    page->vir_w = DIALOG_WIN_FB_W;
    page->win_id = APP_CLOCK_WIN_1;
    page->win_layer = WIN_TOP_LAYER;
    page->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    page->fblen = page->vir_w * page->h * (DIALOG_WIN_COLOR_DEPTH >> 3);
    page->ver_offset = -(DIALOG_WIN_YRES - DIALOG_BUTTON_HEIGHT);
    page->exit_side = EXIT_SIDE_TOP;
}
