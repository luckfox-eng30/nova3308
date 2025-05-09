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

/*
 **************************************************************************************************
 *
 * initial
 *
 **************************************************************************************************
 */
static const char *icons_path[2][3] =
{
    {
        "L:"ICONS_PATH"/alarm_clock.bin",
        "L:"ICONS_PATH"/sleep.bin",
        "L:"ICONS_PATH"/no_disturb.bin",
    },
    // {
    //     "L:"ICONS_PATH"/screen_display.bin",
    //     "L:"ICONS_PATH"/flashlight.bin",
    //     "L:"ICONS_PATH"/find_phone.bin",
    // },
    // {
    //     "L:"ICONS_PATH"/activity_record.bin",
    //     "L:"ICONS_PATH"/exercise_record.bin",
    //     "L:"ICONS_PATH"/training_status.bin",
    // },
    {
        "L:"ICONS_PATH"/pressure.bin",
        "L:"ICONS_PATH"/user_guide.bin",
        "L:"ICONS_PATH"/about_watche.bin"
    }
};
static const char *icons_txt[2][3] =
{
    {
        " Backlight",   //" Alarm",
        " Time",        //" Sleep",
        " Undisturb",
    },
    // {
    //     " Screen",
    //     " FlashLight",
    //     " Find phone",
    // },
    // {
    //     " Activity",
    //     " Exercise",
    //     " Training",
    // },
    {
        " Pressure",
        " User Guide",
        " About"
    }
};

static rt_err_t app_func_setting_excute(void *param)
{
    struct app_page_data_t *page = g_setting_page;
    struct app_setting_private *pdata = page->private;

    app_setting_init(pdata->setting_id);
    app_setting_enter(pdata->setting_id);

    return RT_EOK;
}
design_cb_t app_func_setting_excute_t = {.cb = app_func_setting_excute,};

rt_err_t app_func_setting_tp_touch_up(void *param)
{
    struct app_main_data_t *maindata = (struct app_main_data_t *)param;
    struct app_page_data_t *page = g_func_page;
    struct app_setting_private *pdata = g_setting_page->private;
    int y = maindata->cur_point[0].y_coordinate;
    int idx;

    idx = (y + page->ver_offset) / page->ver_step - 1;
    if (idx < APP_SETTING_BACKLIGHT || idx > APP_SETTING_COMMON)
        return RT_EOK;

    pdata->setting_id = idx;

    app_design_request(0, &app_func_setting_excute_t, RT_NULL);

    return RT_EOK;
}

struct app_touch_cb_t app_setting_touch_cb =
{
    .tp_touch_up = app_func_setting_tp_touch_up,
};

void app_func_setting_exit(void)
{
    app_func_common_exit();
    app_func_revert_touch_ops(&app_setting_touch_cb);
}

void app_func_setting_enter(void *param)
{
    app_func_merge_touch_ops(&app_setting_touch_cb);
    app_func_show(param);
}

void app_func_setting_init(void *param)
{
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *pdata = page->private;
    struct app_lvgl_label_design title;
    struct app_lvgl_iconlist_design iconlist;
    uint32_t padding = 0;

    /* framebuffer malloc */
    page->fblen = MENU_WIN_FB_W * MENU_WIN_FB_H * (MENU_WIN_COLOR_DEPTH >> 3);
    page->fb   = (rt_uint8_t *)rt_malloc_psram(page->fblen);
    RT_ASSERT(page->fb != RT_NULL);
    rt_memset((void *)page->fb, 0x0, page->fblen);

    pdata->alpha_win = 0;
    page->w = MENU_WIN_FB_W;
    page->h = MENU_WIN_FB_H;
    page->vir_w = MENU_WIN_FB_W;
    page->ver_offset = 0;
    page->hor_offset = 0;
    page->ver_step = MENU_WIN_YRES / 3;

    title.txt = "Setting";
    title.ping_pong = 0;
    title.font = &lv_font_montserrat_44;
    title.align = LV_LABEL_ALIGN_CENTER;
    title.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    title.img[0].width = MENU_WIN_XRES;
    title.img[0].height = lv_font_montserrat_44.line_height;
    title.img[0].stride = MENU_WIN_FB_W * (MENU_WIN_COLOR_DEPTH >> 3);
    title.img[0].pdata = page->fb + (MENU_WIN_YRES / 3 - 44) / 2 * MENU_WIN_XRES * (MENU_WIN_COLOR_DEPTH >> 3);
    app_lv_label_design(&title);

    padding = MENU_WIN_XRES * (MENU_WIN_YRES / 3) * (MENU_WIN_COLOR_DEPTH >> 3);
    for (int i = 0; i < 2; i++)
    {
        iconlist.icons_num = 3;
        for (int j = 0; j < iconlist.icons_num; j++)
        {
            iconlist.path[j] = (char *)icons_path[i][j];
            iconlist.txt[j] = (char *)icons_txt[i][j];
        }
        iconlist.img[0].width = MENU_WIN_XRES;
        iconlist.img[0].height = MENU_WIN_YRES;
        iconlist.img[0].stride = MENU_WIN_FB_W * (MENU_WIN_COLOR_DEPTH >> 3);
        iconlist.img[0].pdata = page->fb + padding + i * MENU_WIN_XRES * MENU_WIN_YRES * (MENU_WIN_COLOR_DEPTH >> 3);
        iconlist.ping_pong = 0;
        app_lv_iconlist_design(&iconlist);
    }

    app_func_set_preview(APP_FUNC_SETTING, page->fb);
}

struct app_func func_setting_ops =
{
    .init = app_func_setting_init,
    .enter = app_func_setting_enter,
    .exit = app_func_setting_exit,
};
