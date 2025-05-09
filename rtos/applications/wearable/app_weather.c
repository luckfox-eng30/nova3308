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

#include "app_main.h"
#include "lib_imageprocess.h"

extern lv_font_t lv_font_simhei_36;
extern lv_font_t lv_font_simhei_72;
/*
 **************************************************************************************************
 *
 * Weather design callback
 *
 **************************************************************************************************
 */
static rt_uint8_t update_all = 1;
static rt_uint8_t sunnys_num = 0;
static image_info_t weather_bg;
static image_info_t watch_sunnys_info[5];
static struct image_st target;

rt_err_t app_weather_init(void *param)
{
    struct image_st *par = (struct image_st *)param;
    rt_uint16_t i;
    img_load_info_t img_load_info;
    char img_name[64];

    target = *par;
    // backup
    rt_memset(&weather_bg, 0, sizeof(image_info_t));
    weather_bg.type  = IMG_TYPE_RAW;
    weather_bg.pixel = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    weather_bg.x     = 0;
    weather_bg.y     = 0;
    weather_bg.w     = WEATHER_WIN_XRES;
    weather_bg.h     = WEATHER_WIN_YRES;
    weather_bg.size  = weather_bg.w * weather_bg.h * (WEATHER_WIN_COLOR_DEPTH >> 3),
    weather_bg.data  = (rt_uint8_t *)rt_malloc_psram(weather_bg.size);
    RT_ASSERT(weather_bg.data != RT_NULL);
    img_load_info.w    = weather_bg.w;
    img_load_info.h    = weather_bg.h;
    img_load_info.name = WEATHER_PATH"/weather_bkg.dta";
    app_load_img(&img_load_info, weather_bg.data, weather_bg.w, weather_bg.h, 0, (WEATHER_WIN_COLOR_DEPTH >> 3));

    //large numbers
    for (i = 0; i < 5; i++)
    {
        rt_memset(&watch_sunnys_info[i], 0, sizeof(image_info_t));
        watch_sunnys_info[i].type  = IMG_TYPE_RAW;
        watch_sunnys_info[i].pixel = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        watch_sunnys_info[i].x     = 0;
        watch_sunnys_info[i].y     = 0;
        watch_sunnys_info[i].w     = 96;
        watch_sunnys_info[i].h     = 96;
        watch_sunnys_info[i].size  = watch_sunnys_info[i].w * watch_sunnys_info[i].h * 2;
        watch_sunnys_info[i].data  = (rt_uint8_t *)rt_malloc_psram(watch_sunnys_info[i].size);
        RT_ASSERT(watch_sunnys_info[i].data != RT_NULL);
        rt_memset(img_name, 0, sizeof(img_name));
        snprintf(img_name, sizeof(img_name), WEATHER_PATH"/dynamic_icon_sunny_%01d.dta", i);
        img_load_info.w    = watch_sunnys_info[i].w;
        img_load_info.h    = watch_sunnys_info[i].h;
        img_load_info.name = (const char *)&img_name;
        app_load_img(&img_load_info, watch_sunnys_info[i].data, watch_sunnys_info[i].w, watch_sunnys_info[i].h, 0, 2);
    }

    return RT_EOK;
}

rt_err_t app_weather_design(void *param)
{
    struct image_st *par = &target;
    rt_uint8_t   *fb     = par->pdata;
    rt_uint32_t  vir_w  = par->stride / format2depth[par->format];
    rt_uint16_t  startx = 0;
    rt_uint16_t  starty = 0;
    image_info_t *img_info = NULL;
    rt_err_t     ret;

    if (update_all)
    {
        if (app_lvgl_init_check() == 0)
            return -RT_ERROR;
        //backup
        img_info = &weather_bg;
        rt_display_img_fill(img_info, fb, vir_w, startx, starty);

        struct app_lvgl_label_design label;
        label.ping_pong = 0;
        label.font = &lv_font_simhei_36;
        label.align = LV_LABEL_ALIGN_CENTER;
        label.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        label.img[0] = *par;

        //region
        startx = 0;
        starty = 40;
        label.txt = "鼓楼区";
        label.img[0].width = CLOCK_WIN_XRES;
        label.img[0].height = lv_font_simhei_36.line_height;
        label.img[0].pdata = fb + (startx + starty * vir_w) * format2depth[par->format];
        ret = app_lv_label_design(&label);
        if (ret != RT_EOK)
        {
            return -RT_ERROR;
        }

        //air quality
        startx = 176;
        starty = 192;
        label.txt = "晴";
        label.align = LV_LABEL_ALIGN_LEFT;
        label.img[0].width = 40;
        label.img[0].height = lv_font_simhei_36.line_height;
        label.img[0].pdata = fb + (startx + starty * vir_w) * format2depth[par->format];
        app_lv_label_design(&label);
        if (ret != RT_EOK)
        {
            return -RT_ERROR;
        }

        startx = 256;
        starty = 192;
        label.txt = "优";
        label.img[0].width = 40;
        label.img[0].height = lv_font_simhei_36.line_height;
        label.img[0].pdata = fb + (startx + starty * vir_w) * format2depth[par->format];
        app_lv_label_design(&label);
        if (ret != RT_EOK)
        {
            return -RT_ERROR;
        }

        startx = 290;
        starty = 192;
        label.txt = "12";
        label.img[0].width = 60;
        label.img[0].height = lv_font_simhei_36.line_height;
        label.img[0].pdata = fb + (startx + starty * vir_w) * format2depth[par->format];
        app_lv_label_design(&label);
        if (ret != RT_EOK)
        {
            return -RT_ERROR;
        }

        //temperature
        startx = 0;
        starty = 260;
        label.align = LV_LABEL_ALIGN_CENTER;
        label.txt = "27℃/18℃";
        label.img[0].width = CLOCK_WIN_XRES;
        label.img[0].height = lv_font_simhei_36.line_height;
        label.img[0].pdata = fb + (startx + starty * vir_w) * format2depth[par->format];
        app_lv_label_design(&label);
        if (ret != RT_EOK)
        {
            return -RT_ERROR;
        }

        //update time
        startx = 0;
        starty = 380;
        label.txt = "更新时间 10:26";
        label.img[0].width = CLOCK_WIN_XRES;
        label.img[0].height = lv_font_simhei_36.line_height;
        label.img[0].pdata = fb + (startx + starty * vir_w) * format2depth[par->format];
        app_lv_label_design(&label);
        if (ret != RT_EOK)
        {
            return -RT_ERROR;
        }

        //temperature large
        startx = 176;
        starty = 118;
        label.txt = "26℃";
        label.font = &lv_font_simhei_72;
        label.img[0].width = 156;
        label.img[0].height = lv_font_simhei_72.line_height;
        label.img[0].pdata = fb + (startx + starty * vir_w) * format2depth[par->format];
        app_lv_label_design(&label);
        if (ret != RT_EOK)
        {
            return -RT_ERROR;
        }
    }
    update_all = 0;

    //sunnys
    startx = 30;
    starty = 118;
    img_info = &watch_sunnys_info[sunnys_num];
    if (++sunnys_num >= 5)
    {
        sunnys_num = 0;
    }
    rt_display_img_fill(img_info, fb, vir_w, startx, starty);

    return RT_EOK;
}