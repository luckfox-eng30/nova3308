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

#include "app_main.h"

#define LV_REGION_X         0
#define LV_REGION_Y         0
#define LV_FB_W             LV_HOR_RES
#define LV_FB_H             LV_VER_RES
#define LV_FB_COLOR_DEPTH   LV_COLOR_DEPTH

static rt_uint8_t  thread_flag = 0;
static rt_thread_t thread;
static rt_event_t  disp_event;
static lv_style_t  main_style;
static lv_obj_t   *obj_main;
static lv_obj_t   *slider_label;
static struct rt_touchpanel_block lv_touch_block;

static rt_uint8_t bl_time_table[4] = {5, 15, 30, 0};

/*
 **************************************************************************************************
 *
 * lvgl design
 *
 **************************************************************************************************
 */
static void slider_event_cb(lv_obj_t *slider, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        char buf[4]; /* max 3 bytes for number plus 1 null terminating byte */
        app_main_data->bl = lv_slider_get_value(slider);
        snprintf(buf, 4, "%u", app_main_data->bl);
        lv_label_set_text(slider_label, buf);
        rt_display_backlight_set(app_main_data->bl);
    }
}

static void dropdown_event_cb(lv_obj_t *obj, lv_event_t event)
{
    if (event == LV_EVENT_VALUE_CHANGED)
    {
        app_main_data->bl_time = bl_time_table[lv_dropdown_get_selected(obj)];
        app_main_set_bl_timeout(app_main_data->bl_time);
    }
}

static void lv_backlight_design(void)
{
    /* Background */
    lv_style_init(&main_style);
    lv_style_set_bg_color(&main_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_radius(&main_style, LV_STATE_DEFAULT, 0/*LV_RADIUS_CIRCLE*/);
    lv_style_set_border_width(&main_style, LV_STATE_DEFAULT, 0);

    obj_main = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_add_style(obj_main, LV_STATE_DEFAULT, &main_style);
    lv_obj_set_size(obj_main, LV_FB_W, LV_FB_H);
    lv_obj_set_pos(obj_main, 0, 0);

    /*Create a normal drop down list*/
    uint16_t sel = 0;
    if (app_main_data->bl_time == 0) sel = 3;
    else if (app_main_data->bl_time <= 5) sel = 0;
    else if (app_main_data->bl_time <= 15) sel = 1;
    else if (app_main_data->bl_time <= 30) sel = 2;
    lv_obj_t *ddlist = lv_dropdown_create(obj_main, NULL);
    lv_dropdown_set_options(ddlist,
                            "5 S\n"
                            "15 S\n"
                            "30 S\n"
                            "Always on");
    lv_obj_set_width(ddlist, 300);
    lv_dropdown_set_selected(ddlist, sel);
    lv_obj_align(ddlist, NULL, LV_ALIGN_IN_TOP_MID, 0, 150);
    lv_obj_set_event_cb(ddlist, dropdown_event_cb);

    lv_obj_t *dd_label = lv_label_create(obj_main, NULL);
    lv_label_set_text(dd_label, "Screen off time:");
    lv_obj_align(dd_label, ddlist, LV_ALIGN_OUT_TOP_LEFT, 0, -20);

    /* Create a slider in the center of the display */
    lv_obj_t *slider = lv_slider_create(obj_main, NULL);
    lv_obj_set_size(slider, 300, 40);
    lv_obj_align(slider, ddlist, LV_ALIGN_OUT_BOTTOM_MID, 0, 120);
    lv_obj_set_event_cb(slider, slider_event_cb);
    lv_slider_set_range(slider, 20, 100);
    lv_slider_set_value(slider, app_main_data->bl, LV_ANIM_OFF);

    /* Create a label below the slider */
    char buf[4];
    rt_memset(buf, 0, 4);
    snprintf(buf, 4, "%u", app_main_data->bl);

    lv_obj_t *bln_label = lv_label_create(obj_main, NULL);
    lv_label_set_text(bln_label, "Brightness:");
    lv_obj_align(bln_label, slider, LV_ALIGN_OUT_TOP_LEFT, 0, -20);

    slider_label = lv_label_create(obj_main, NULL);
    lv_label_set_text(slider_label, buf);
    lv_obj_align(slider_label, slider, LV_ALIGN_OUT_TOP_RIGHT, -10, -20);
    lv_obj_set_auto_realign(slider_label, true);

    lv_refr_now(lv_disp_get_default());

    lv_obj_invalidate(obj_main);
}

/*
 **************************************************************************************************
 *
 * touch process
 *
 **************************************************************************************************
 */
/**
 * lvgl touch callback.
 */
static rt_uint8_t touch_func_sel = 0;
static rt_err_t lv_touch_cb(struct rt_touch_data *point, rt_uint8_t num)
{
    struct rt_touch_data *p = &point[0];
    struct rt_touchpanel_block *b = &lv_touch_block;

    if (RT_EOK != rt_touchpoint_is_valid(p, b))
    {
        return -RT_ERROR;
    }

    if (b->event == RT_TOUCH_EVENT_DOWN)
    {
        touch_func_sel = 0;
        if ((app_main_data->bl_en != 0) &&
                ((p->x_coordinate - b->x) < TOUCH_REGION_W - 30))
        {
            touch_func_sel = 1;
        }
    }

    if (touch_func_sel != 0)
    {
        app_main_keep_screen_on();
        if (b->event == RT_TOUCH_EVENT_DOWN)
        {
            littlevgl2rtt_send_input_event(p->x_coordinate - b->x, p->y_coordinate - b->y, LITTLEVGL2RTT_INPUT_DOWN);
        }
        else if (b->event == RT_TOUCH_EVENT_MOVE)
        {
            littlevgl2rtt_send_input_event(p->x_coordinate - b->x, p->y_coordinate - b->y, LITTLEVGL2RTT_INPUT_MOVE);
        }
        else if (b->event == RT_TOUCH_EVENT_UP)
        {
            littlevgl2rtt_send_input_event(p->x_coordinate - b->x, p->y_coordinate - b->y, LITTLEVGL2RTT_INPUT_UP);
        }
    }
    else
    {
        app_main_touch_process(point, num);
    }

    return RT_EOK;
}

static void lv_touch_block_init(struct rt_touchpanel_block *block)
{
    struct rt_device_graphic_info *info = &app_main_data->disp->info;

    rt_memset(block, 0, sizeof(struct rt_touchpanel_block));

    block->x = LV_REGION_X + ((info->width  - LV_FB_W) / 2);
    block->y = LV_REGION_Y + ((info->height - LV_FB_H) / 2);
    block->w = LV_FB_W;
    block->h = LV_FB_H;
    block->name = "backlight";
    block->callback = lv_touch_cb;
}

/*
 **************************************************************************************************
 *
 * refresh process
 *
 **************************************************************************************************
 */
/**
 * app clock display refresh request: request send data to LCD pannel.
 */
static rt_err_t lv_refr_done(void)
{
    return (rt_event_send(disp_event, EVENT_REFR_DONE));
}

static rt_err_t lv_refr_request(struct rt_display_mq_t *disp_mq)
{
    rt_err_t ret;

    //request refresh display data to Pannel
    disp_mq->disp_finish = lv_refr_done;
    ret = rt_mq_send(app_main_data->disp->disp_mq, disp_mq, sizeof(struct rt_display_mq_t));
    RT_ASSERT(ret == RT_EOK);

    //wait refresh done
    rt_uint32_t event;
    ret = rt_event_recv(disp_event, EVENT_REFR_DONE,
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                        RT_WAITING_FOREVER, &event);
    RT_ASSERT(ret == RT_EOK);

    return RT_EOK;
}

/**
 * home region refresh.
 */
static void lv_lcd_flush(void)
{
    struct app_page_data_t *page = g_setting_page;
    struct rt_display_mq_t disp_mq;
    struct rt_display_config *wincfg = &disp_mq.win[page->win_id];
    struct rt_device_graphic_info *info = &app_main_data->disp->info;

    rt_memset(&disp_mq, 0, sizeof(struct rt_display_mq_t));
    disp_mq.win[0].zpos = page->win_layer;
    disp_mq.cfgsta |= (0x01 << page->win_id);

    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    wincfg->lut     = RT_NULL;
    wincfg->lutsize = 0;
    wincfg->winId   = page->win_id;
    wincfg->fb      = g_lvdata->fb;
    wincfg->fblen   = g_lvdata->fblen;
    wincfg->w       = LV_FB_W;
    wincfg->h       = LV_FB_H;
    wincfg->x       = LV_REGION_X + ((info->width  - LV_FB_W) / 2);
    wincfg->y       = LV_REGION_Y + ((info->height - LV_FB_H) / 2);
    wincfg->ylast   = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->y % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);

    lv_refr_request(&disp_mq);
}

/*
 **************************************************************************************************
 *
 * main process
 *
 **************************************************************************************************
 */
static void lv_backlight_thread(void *p)
{
    /* Creat event for lcd display */
    disp_event = rt_event_create("bl_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(disp_event != RT_NULL);

    /* register lvgl send lcd I/F */
    rtlvgl_fb_monitor_cb_register(lv_lcd_flush);

    /* Re-register touch process for lvgl using */
    rt_touchpanel_block_unregister(&app_main_data->touch_block);

    lv_touch_block_init(&lv_touch_block);
    rt_touchpanel_block_register(&lv_touch_block);

    while (1)
    {
        rt_thread_mdelay(10);
        lv_task_handler();

        if (thread_flag)
        {
            break;
        }
    }

    rtlvgl_fb_monitor_cb_unregister(lv_lcd_flush);
    rt_event_delete(disp_event);

    lv_obj_clean_style_list(obj_main, LV_STATE_DEFAULT);
    lv_obj_del(obj_main);
    save_app_info(app_main_data);
}

/*
 **************************************************************************************************
 *
 * func API
 *
 **************************************************************************************************
 */
void func_backlight_enter(void *param)
{
    /* Re-register touch process for common using */
    app_setting_show(param);

    /* Creat thread for lvgl task running */
    thread_flag = 0;
    thread = rt_thread_create("funcbl", lv_backlight_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(thread != RT_NULL);
    rt_thread_startup(thread);
}

void func_backlight_init(void *param)
{
    struct app_page_data_t *page = g_setting_page;

    page->w = LV_FB_W;
    page->h = LV_FB_H;
    page->vir_w = LV_FB_W;
    page->ver_offset = 0;
    page->hor_offset = 0;
    page->fblen = g_lvdata->fblen;
    page->fb    = g_lvdata->fb;

    // first display design
    lv_backlight_design();

    // copy data to app_setting_main
    app_func_set_preview(APP_FUNC_BREATH, page->fb);
}

void func_backlight_exit(void)
{
    rt_touchpanel_block_unregister(&lv_touch_block);
    rt_touchpanel_block_register(&app_main_data->touch_block);
    thread_flag = 1;
}

void func_backlight_pause(void)
{
    rtlvgl_fb_monitor_cb_unregister(lv_lcd_flush);
}

void func_backlight_resume(void)
{
    rtlvgl_fb_monitor_cb_register(lv_lcd_flush);    
}

struct app_func func_backlight_ops =
{
    .init = func_backlight_init,
    .enter = func_backlight_enter,
    .exit = func_backlight_exit,
    .pause = func_backlight_pause,
    .resume = func_backlight_resume,
};
