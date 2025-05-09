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
static lv_style_t  roller_style;
static lv_obj_t   *obj_main;
static lv_obj_t   *obj_hour;
static lv_obj_t   *obj_min;
static struct rt_touchpanel_block lv_touch_block;

static const char *hour_str = "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n"
                              "10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n"
                              "20\n21\n22\n23";
static const char *min_str =  "00\n01\n02\n03\n04\n05\n06\n07\n08\n09\n"
                              "10\n11\n12\n13\n14\n15\n16\n17\n18\n19\n"
                              "20\n21\n22\n23\n24\n25\n26\n27\n28\n29\n"
                              "30\n31\n32\n33\n34\n35\n36\n37\n38\n39\n"
                              "40\n41\n42\n43\n44\n45\n46\n47\n48\n49\n"
                              "50\n51\n52\n53\n54\n55\n56\n57\n58\n59";

/*
 **************************************************************************************************
 *
 * lvgl design
 *
 **************************************************************************************************
 */
static void lv_time_set_done(void)
{
    struct tm time;

    app_main_get_time(&app_main_data->tmr_data);
    memcpy(&time, app_main_data->tmr_data, sizeof(struct tm));

    char buf[8];
    lv_roller_get_selected_str(obj_hour, buf, sizeof(buf));
    time.tm_hour = app_str2num(buf, 2);

    lv_roller_get_selected_str(obj_min, buf, sizeof(buf));
    time.tm_min = app_str2num(buf, 2);

    app_main_set_time(&time);
}

static void lv_time_set_design(void)
{
    struct tm *time;
    lv_obj_t *label;

    app_main_get_time(&time);
    /* Background */
    lv_style_init(&main_style);
    lv_style_set_bg_color(&main_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_radius(&main_style, LV_STATE_DEFAULT, 0/*LV_RADIUS_CIRCLE*/);
    lv_style_set_border_width(&main_style, LV_STATE_DEFAULT, 0);

    obj_main = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_add_style(obj_main, LV_STATE_DEFAULT, &main_style);
    lv_obj_set_size(obj_main, LV_FB_W, LV_FB_H);
    lv_obj_set_pos(obj_main, 0, 0);

    /* Time setting */
    lv_style_init(&roller_style);
    lv_style_set_text_font(&roller_style, LV_STATE_DEFAULT, &lv_font_montserrat_44);

    obj_hour = lv_roller_create(obj_main, NULL);
    lv_obj_add_style(obj_hour, LV_STATE_DEFAULT, &roller_style);
    lv_roller_set_options(obj_hour, hour_str, LV_ROLLER_MODE_INIFINITE);
    lv_roller_set_visible_row_count(obj_hour, 2);
    lv_obj_align(obj_hour, obj_main, LV_ALIGN_CENTER, -55, 10);
    lv_roller_set_selected(obj_hour, time->tm_hour - app_str2num(hour_str, 2), LV_ANIM_OFF);

    label = lv_label_create(obj_main, NULL);
    lv_label_set_text(label, "Hour");
    lv_obj_align(label, obj_hour, LV_ALIGN_OUT_TOP_MID, 0, -30);

    obj_min = lv_roller_create(obj_main, NULL);
    lv_obj_add_style(obj_min, LV_STATE_DEFAULT, &roller_style);
    lv_roller_set_options(obj_min, min_str, LV_ROLLER_MODE_INIFINITE);
    lv_roller_set_visible_row_count(obj_min, 2);
    lv_obj_align(obj_min, obj_main, LV_ALIGN_CENTER, 55, 10);
    lv_roller_set_selected(obj_min, time->tm_min - app_str2num(min_str, 2), LV_ANIM_OFF);

    label = lv_label_create(obj_main, NULL);
    lv_label_set_text(label, "Min");
    lv_obj_align(label, obj_min, LV_ALIGN_OUT_TOP_MID, 0, -30);

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
    block->name = "settime";
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
static void lv_time_set_thread(void *p)
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

    lv_time_set_done();
    lv_obj_clean_style_list(obj_hour, LV_STATE_DEFAULT);
    lv_obj_clean_style_list(obj_min, LV_STATE_DEFAULT);
    lv_obj_clean_style_list(obj_main, LV_STATE_DEFAULT);
    lv_obj_del(obj_main);
}

/*
 **************************************************************************************************
 *
 * func API
 *
 **************************************************************************************************
 */
void func_time_set_enter(void *param)
{
    /* Re-register touch process for common using */
    app_setting_show(param);

    /* Creat thread for lvgl task running */
    thread_flag = 0;
    thread = rt_thread_create("functime", lv_time_set_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(thread != RT_NULL);
    rt_thread_startup(thread);
}

void func_time_set_init(void *param)
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
    lv_time_set_design();

    // copy data to app_setting_main
    app_func_set_preview(APP_FUNC_BAROMETER, page->fb);
}

void func_time_set_exit(void)
{
    rt_touchpanel_block_unregister(&lv_touch_block);
    rt_touchpanel_block_register(&app_main_data->touch_block);
    thread_flag = 1;
}

void func_time_set_pause(void)
{
    rtlvgl_fb_monitor_cb_unregister(lv_lcd_flush);
}

void func_time_set_resume(void)
{
    rtlvgl_fb_monitor_cb_register(lv_lcd_flush);    
}

struct app_func func_time_ops =
{
    .init = func_time_set_init,
    .enter = func_time_set_enter,
    .exit = func_time_set_exit,
    .pause = func_time_set_pause,
    .resume = func_time_set_resume,
};
