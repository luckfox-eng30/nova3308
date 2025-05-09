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
 * Global declaration
 *
 **************************************************************************************************
 */
static rt_err_t app_digital_clock_style0_init(void);
static rt_err_t app_digital_clock_style0_design(void *param);
static rt_err_t app_analog_clock_style_design(void *param);

typedef struct
{
    rt_err_t (*cb)(void);
    uint8_t inited;
} app_clock_init_cb_t;

typedef struct
{
    rt_err_t (*cb)(void *param);
} app_clock_design_cb_t;

static app_clock_init_cb_t clock_init_func[CLOCK_STYLE_MAX_NUM] =
{
    {app_digital_clock_style0_init, 0},
    {RT_NULL, 0},
    {RT_NULL, 0},
    {RT_NULL, 0},
};

static const app_clock_design_cb_t clock_design_func[CLOCK_STYLE_MAX_NUM] =
{
    {app_digital_clock_style0_design,},
    {app_analog_clock_style_design,},
    {app_analog_clock_style_design,},
    {app_analog_clock_style_design,},
};

static const img_load_info_t img_clk_bkg[CLOCK_STYLE_MAX_NUM] =
{
    { 0, 0, RT_NULL},
    { DISP_XRES, DISP_YRES, USERDATA_PATH"img_clk0_bkg.dta"},
    { DISP_XRES, DISP_YRES, USERDATA_PATH"img_clk1_bkg.dta"},
    { DISP_XRES, DISP_YRES, USERDATA_PATH"img_clk2_bkg.dta"},
};

static struct image_st target;

/*
 **************************************************************************************************
 *
 * Analog clock design
 *
 **************************************************************************************************
 */

#define CLOCK_SHOW_FPS      0
#define NEW_ROTATE_ALGORITHM
#ifdef NEW_ROTATE_ALGORITHM
#define CLOCK_FAST_RGB565   1
#else
#define CLOCK_FAST_RGB565   0
#endif

struct clock_needle_img_t
{
    image_info_t *info;
    float cx;
    float cy;
};

struct clock_needle_grp_t
{
    struct clock_needle_img_t hour;
    struct clock_needle_img_t min;
    struct clock_needle_img_t sec;
    struct clock_needle_img_t cen;
};

extern image_info_t img_clk0_hour_info;
extern image_info_t img_clk0_min_info;
extern image_info_t img_clk0_sec_info;
extern image_info_t img_clk0_center_info;

extern image_info_t img_clk1_hour_info;
extern image_info_t img_clk1_min_info;
extern image_info_t img_clk1_sec_info;

extern image_info_t img_clk2_hour_info;
extern image_info_t img_clk2_min_info;
extern image_info_t img_clk2_sec_info;

#ifndef NEW_ROTATE_ALGORITHM
static const struct clock_needle_grp_t needle_img[CLOCK_STYLE_MAX_NUM] =
{
    //{{NULL, 0.0, 0.0}, {NULL, 0.0, 0.0}, {NULL, 0.0, 0.0}, {NULL, 0.0, 0.0}},
    {{&img_clk0_hour_info, 5.0, 5.0}, {&img_clk0_min_info, 2.0, 2.0}, {&img_clk0_sec_info, 2.0, 2.0}, {&img_clk0_center_info, 6.0, 6.0}},
    {{&img_clk0_hour_info, 5.0, 5.0}, {&img_clk0_min_info, 2.0, 2.0}, {&img_clk0_sec_info, 2.0, 2.0}, {&img_clk0_center_info, 6.0, 6.0}},
    {{&img_clk0_hour_info, 5.0, 5.0}, {&img_clk0_min_info, 2.0, 2.0}, {&img_clk0_sec_info, 2.0, 2.0}, {&img_clk0_center_info, 6.0, 6.0}},
    {{&img_clk0_hour_info, 5.0, 5.0}, {&img_clk0_min_info, 2.0, 2.0}, {&img_clk0_sec_info, 2.0, 2.0}, {&img_clk0_center_info, 6.0, 6.0}},
};
#else
static const struct clock_needle_grp_t needle_img[CLOCK_STYLE_MAX_NUM] =
{
    //{{NULL, 0.0, 0.0}, {NULL, 0.0, 0.0}, {NULL, 0.0, 0.0}, {NULL, 0.0, 0.0}},
    {{&img_clk0_hour_info, 6.0, 6.0}, {&img_clk0_min_info, 6.0, 6.0}, {&img_clk0_sec_info, 19.0, 6.0}, {&img_clk0_center_info, 6.0, 6.0}},
    {{&img_clk0_hour_info, 6.0, 6.0}, {&img_clk0_min_info, 6.0, 6.0}, {&img_clk0_sec_info, 19.0, 6.0}, {&img_clk0_center_info, 6.0, 6.0}},
    {{&img_clk1_hour_info, -7.0, 5.0}, {&img_clk1_min_info, -5.0, 5.5}, {&img_clk1_sec_info, 9.5, 9.5}, {&img_clk0_center_info, 6.0, 6.0}},
    {{&img_clk2_hour_info, -10.0, 6.0}, {&img_clk2_min_info, -10.0, 4.0}, {&img_clk2_sec_info, 6.0, 6.0}, {&img_clk0_center_info, 6.0, 6.0}},
};
#endif

#if CLOCK_FAST_RGB565
struct pre_info
{
    short info[DISP_YRES][2];
    uint8_t hour;
    uint8_t min;
    uint8_t style;
};
struct pre_info preInfo = {{{0}, {0}}, 0xff, 0xff};
static struct rotateimage_st pbg = {0, 0, 0, 0, 0, NULL};
static uint8_t *bg_buf = NULL;
#endif

/*
 * =======================================================
 * Analog clock design
 * =======================================================
 */
static rt_err_t app_analog_clock_style_design(void *param)
{
    struct image_st *par = (struct image_st *)&target;
    rt_uint8_t  *fb   = par->pdata;
#if !CLOCK_FAST_RGB565
    rt_uint32_t vir_w = par->stride / format2depth[par->format];
#endif
    struct tm *time       = app_main_data->tmr_data;
    rt_int8_t    style_id = app_main_data->clock_style;
    rt_uint16_t  xstart   = 0;
    int32_t hour;
    int32_t angle;

    image_info_t *img_hour = needle_img[style_id].hour.info;
    image_info_t *img_min  = needle_img[style_id].min.info;
    image_info_t *img_sec  = needle_img[style_id].sec.info;
#ifndef NEW_ROTATE_ALGORITHM
    image_info_t *img_cen  = needle_img[style_id].cen.info;
#endif

#if CLOCK_SHOW_FPS > 0
    uint32_t st, et;

    st = HAL_GetTick();
#endif

#ifndef NEW_ROTATE_ALGORITHM
    app_load_img((img_load_info_t *)&img_clk_bkg[style_id], (rt_uint8_t *)fb,
                 vir_w, par->height, xstart, format2depth[par->format]);

    //draw clock needles
    rt_uint16_t xoffset = (par->width / 2) + xstart;
    rt_uint16_t yoffset = (par->width / 2);

    //draw hour line
    hour = time->tm_hour;
    if (hour >= 12) hour -= 12;
    angle = hour * (360 / 12) + (time->tm_min * 30) / 60 - 90;
    if (angle < 0) angle += 360;
    rt_display_rotate_16bit((float)angle, img_hour->w, img_hour->h, (unsigned short *)img_hour->data,
                            (unsigned short *)((uint32_t)fb + format2depth[par->format] * (yoffset * vir_w + xoffset)),
                            vir_w, 0, img_hour->h / 2);

    //draw min line
    angle = time->tm_min * (360 / 60) - 90;
    if (angle < 0) angle += 360;
    rt_display_rotate_16bit((float)angle, img_min->w, img_min->h, (unsigned short *)img_min->data,
                            (unsigned short *)((uint32_t)fb + format2depth[par->format] * (yoffset * vir_w + xoffset)),
                            vir_w, 0, img_min->h / 2);

    //draw second line
    angle  = time->tm_sec * (360 / 60) - 90;
    if (angle < 0) angle += 360;
    rt_display_rotate_16bit((float)angle, img_sec->w, img_sec->h, (unsigned short *)img_sec->data,
                            (unsigned short *)((uint32_t)fb + format2depth[par->format] * (yoffset * vir_w + xoffset)),
                            vir_w, 0, img_sec->h / 2);

    //draw centre
    yoffset  -= img_cen->h / 2;
    xoffset  -= img_cen->w / 2;
    rt_display_img_fill(img_cen, fb, vir_w, xoffset, yoffset);
#else
    struct rotateimage_st ps, pd;
    float h_cx = needle_img[style_id].hour.cx;
    float h_cy = needle_img[style_id].hour.cy;
    float m_cx = needle_img[style_id].min.cx;
    float m_cy = needle_img[style_id].min.cy;
    float s_cx = needle_img[style_id].sec.cx;
    float s_cy = needle_img[style_id].sec.cy;

    pd.width = par->width;
    pd.height = par->height;
    pd.stride = par->stride;
    pd.cx = pd.width / 2;
    pd.cy = pd.height / 2;
    pd.pdata = fb + xstart * format2depth[par->format];

#if !CLOCK_FAST_RGB565
    app_load_img((img_load_info_t *)&img_clk_bkg[style_id], (rt_uint8_t *)fb, vir_w, par->height, xstart, format2depth[par->format]);

    ps.width = img_hour->w;
    ps.height = img_hour->h;
    ps.stride = ps.width * 4;
    ps.cx = h_cx;
    ps.cy = h_cy;
    ps.pdata = img_hour->data;

    hour = time->tm_hour;
    if (hour >= 12)
    {
        hour -= 12;
    }
    angle = hour * (360 / 12) + (time->tm_min * 30) / 60 - 90;
    if (angle < 0)
    {
        angle += 360;
    }
    rk_rotate_process_16bit(&ps, &pd, (360 - angle % 360));

    ps.width = img_min->w;
    ps.height = img_min->h;
    ps.stride = ps.width * 4;
    ps.cx = m_cx;
    ps.cy = m_cy;
    ps.pdata = img_min->data;

    angle = time->tm_min * (360 / 60) - 90;
    if (angle < 0)
    {
        angle += 360;
    }
    rk_rotate_process_16bit(&ps, &pd, (360 - angle % 360));
#else
    pbg.pdata = bg_buf;
    if (preInfo.hour != time->tm_hour || preInfo.min != time->tm_min || preInfo.style != style_id)
    {
        preInfo.hour = time->tm_hour;
        preInfo.min = time->tm_min;
        preInfo.style = style_id;
        memset(preInfo.info[0], 0, par->height * 2 * sizeof(short));
        pbg.width = par->width;
        pbg.height = par->height;
        pbg.stride = par->width * format2depth[par->format];
        pbg.cx = pbg.width / 2;
        pbg.cy = pbg.height / 2;

        ps.width = img_hour->w;
        ps.height = img_hour->h;
        ps.stride = ps.width * 4;
        ps.cx = h_cx;
        ps.cy = h_cy;
        ps.pdata = img_hour->data;

        if (pbg.pdata == NULL)
        {
            bg_buf = pbg.pdata = rt_malloc_psram(par->stride * par->height);
            RT_ASSERT(bg_buf != NULL);
        }
        app_load_img((img_load_info_t *)&img_clk_bkg[style_id], (rt_uint8_t *)pbg.pdata,
                     par->width, par->height, 0, format2depth[par->format]);
        hour = time->tm_hour;
        if (hour >= 12)
        {
            hour -= 12;
        }
        angle = hour * (360 / 12) + (time->tm_min * 30) / 60 - 90;
        if (angle < 0)
        {
            angle += 360;
        }
        rk_rotate_process_16bit(&ps, &pbg, (360 - angle % 360));

        ps.width = img_min->w;
        ps.height = img_min->h;
        ps.stride = ps.width * 4;
        ps.cx = m_cx;
        ps.cy = m_cy;
        ps.pdata = img_min->data;

        angle = time->tm_min * (360 / 60) - 90;
        if (angle < 0)
        {
            angle += 360;
        }
        rk_rotate_process_16bit(&ps, &pbg, (360 - angle % 360));
        for (int i = 0; i < pd.height; i++)
            memcpy(pd.pdata + i * pd.stride, pbg.pdata + i * pbg.stride,
                   pbg.width * format2depth[par->format]);
    }
#endif
    ps.width = img_sec->w;
    ps.height = img_sec->h;
    ps.stride = ps.width * 4;
    ps.cx = s_cx;
    ps.cy = s_cy;
    ps.pdata = img_sec->data;

    angle = time->tm_sec * (360 / 60) - 90;
    if (angle < 0)
    {
        angle += 360;
    }
#if !CLOCK_FAST_RGB565
    rk_rotate_process_16bit(&ps, &pd, (360 - angle % 360));
#else
    rk_rotate_process_16bitfast(&ps, &pbg, &pd, preInfo.info[0], (360 - angle % 360));
#endif
#endif

#if CLOCK_SHOW_FPS > 0
    et = HAL_GetTick();
    rt_kprintf("FPS:%d(%dms)\n", 1000 / (et - st), et - st);
#endif

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * Digital style 0 clock design
 *
 **************************************************************************************************
 */
static image_info_t watch_bg;
static image_info_t watch_num_dot;
static image_info_t watch_num_l_num_info[10];
static image_info_t watch_num_s_num_info[10];

static rt_err_t app_digital_clock_style0_init(void)
{
    rt_uint16_t i;
    img_load_info_t img_load_info;
    char img_name[64];

    // backup
    rt_memset(&watch_bg, 0, sizeof(image_info_t));
    watch_bg.type  = IMG_TYPE_RAW;
    watch_bg.pixel = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    watch_bg.x     = 0;
    watch_bg.y     = 0;
    watch_bg.w     = CLOCK_WIN_XRES;
    watch_bg.h     = CLOCK_WIN_YRES;
    watch_bg.size  = watch_bg.w * watch_bg.h * (CLOCK_WIN_COLOR_DEPTH >> 3),
    watch_bg.data  = (rt_uint8_t *)rt_malloc_psram(watch_bg.size);
    RT_ASSERT(watch_bg.data != RT_NULL);
    img_load_info.w    = watch_bg.w;
    img_load_info.h    = watch_bg.h;
    img_load_info.name = WATCH_PATH"/watch_bg.dta";
    app_load_img(&img_load_info, watch_bg.data, watch_bg.w, watch_bg.h, 0, (CLOCK_WIN_COLOR_DEPTH >> 3));

    // backup
    rt_memset(&watch_num_dot, 0, sizeof(image_info_t));
    watch_num_dot.type  = IMG_TYPE_RAW;
    watch_num_dot.pixel = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    watch_num_dot.x     = 0;
    watch_num_dot.y     = 0;
    watch_num_dot.w     = 20;
    watch_num_dot.h     = 36;
    watch_num_dot.size  = watch_num_dot.w * watch_num_dot.h * 2,
    watch_num_dot.data  = (rt_uint8_t *)rt_malloc_psram(watch_num_dot.size);
    RT_ASSERT(watch_num_dot.data != RT_NULL);
    img_load_info.w    = watch_num_dot.w;
    img_load_info.h    = watch_num_dot.h;
    img_load_info.name = WATCH_PATH"/watch_num_dot.dta";
    app_load_img(&img_load_info, watch_num_dot.data, watch_num_dot.w, watch_num_dot.h, 0, 2);

    //large numbers
    for (i = 0; i < 10; i++)
    {
        rt_memset(&watch_num_l_num_info[i], 0, sizeof(image_info_t));
        watch_num_l_num_info[i].type  = IMG_TYPE_RAW;
        watch_num_l_num_info[i].pixel = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        watch_num_l_num_info[i].x     = 0;
        watch_num_l_num_info[i].y     = 0;
        watch_num_l_num_info[i].w     = 90;
        watch_num_l_num_info[i].h     = 128;
        watch_num_l_num_info[i].size  = watch_num_l_num_info[i].w * watch_num_l_num_info[i].h * 2,
                               watch_num_l_num_info[i].data  = (rt_uint8_t *)rt_malloc_psram(watch_num_l_num_info[i].size);
        RT_ASSERT(watch_num_l_num_info[i].data != RT_NULL);
        rt_memset(img_name, 0, sizeof(img_name));
        snprintf(img_name, sizeof(img_name), WATCH_PATH"/watch_num_l_%01d.dta", i);
        img_load_info.w    = watch_num_l_num_info[i].w;
        img_load_info.h    = watch_num_l_num_info[i].h;
        img_load_info.name = (const char *)&img_name;
        app_load_img(&img_load_info, watch_num_l_num_info[i].data, watch_num_l_num_info[i].w, watch_num_l_num_info[i].h, 0, 2);
    }

    //small numbers
    for (i = 0; i < 10; i++)
    {
        rt_memset(&watch_num_s_num_info[i], 0, sizeof(image_info_t));
        watch_num_s_num_info[i].type  = IMG_TYPE_RAW;
        watch_num_s_num_info[i].pixel = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        watch_num_s_num_info[i].x     = 0;
        watch_num_s_num_info[i].y     = 0;
        watch_num_s_num_info[i].w     = 56;
        watch_num_s_num_info[i].h     = 76;
        watch_num_s_num_info[i].size  = watch_num_s_num_info[i].w * watch_num_s_num_info[i].h * 2,
                               watch_num_s_num_info[i].data  = (rt_uint8_t *)rt_malloc_psram(watch_num_s_num_info[i].size);
        RT_ASSERT(watch_num_s_num_info[i].data != RT_NULL);
        rt_memset(img_name, 0, sizeof(img_name));
        snprintf(img_name, sizeof(img_name), WATCH_PATH"/watch_num_s_%01d.dta", i);
        img_load_info.w    = watch_num_s_num_info[i].w;
        img_load_info.h    = watch_num_s_num_info[i].h;
        img_load_info.name = (const char *)&img_name;
        app_load_img(&img_load_info, watch_num_s_num_info[i].data, watch_num_s_num_info[i].w, watch_num_s_num_info[i].h, 0, 2);
    }

    return RT_EOK;
}

static rt_err_t app_digital_clock_style0_design(void *param)
{
    struct image_st *par = (struct image_st *)&target;
    rt_uint8_t  *fb   = par->pdata;
    rt_uint32_t vir_w = par->stride / format2depth[par->format];
    struct tm *time   = app_main_data->tmr_data;
    rt_int8_t    style_id = app_main_data->clock_style;
    rt_uint16_t  startx, deltax, xoffset  = 0;
    rt_uint16_t  starty, deltay, yoffset  = 0;
    image_info_t *img_info = NULL;
    preInfo.style = style_id;

    starty = 0;
    startx = (watch_bg.w - (watch_num_l_num_info[0].w * 2 + watch_num_dot.w + watch_num_s_num_info[0].w * 2)) / 2;

    //backup
    img_info = &watch_bg;
    rt_display_img_fill(img_info, fb, vir_w, xoffset + 0, yoffset + 0);

    //hour
    deltax   = 0;
    img_info = &watch_num_l_num_info[time->tm_hour / 10];
    deltay    = (watch_bg.h - img_info->h) / 2;
    rt_display_img_fill(img_info, fb, vir_w, xoffset + startx + deltax, yoffset + starty + deltay);

    deltax   += img_info->w;
    img_info  = &watch_num_l_num_info[time->tm_hour % 10];
    deltay    = (watch_bg.h - img_info->h) / 2;
    rt_display_img_fill(img_info, fb, vir_w, xoffset + startx + deltax, yoffset + starty + deltay);

    //dot
    deltax   += img_info->w;
    img_info = &watch_num_dot;
    deltay    = (watch_bg.h - img_info->h) / 2;
    rt_display_img_fill(img_info, fb, vir_w, xoffset + startx + deltax, yoffset + starty + deltay);

    //min
    deltax   += img_info->w;
    img_info = &watch_num_s_num_info[time->tm_min / 10];
    deltay    = (watch_bg.h - img_info->h) / 2;
    rt_display_img_fill(img_info, fb, vir_w, xoffset + startx + deltax, yoffset + starty + deltay);

    deltax   += img_info->w;
    img_info = &watch_num_s_num_info[time->tm_min % 10];
    deltay    = (watch_bg.h - img_info->h) / 2;
    rt_display_img_fill(img_info, fb, vir_w, xoffset + startx + deltax, yoffset + starty + deltay);

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * Clock design callback
 *
 **************************************************************************************************
 */
rt_err_t app_clock_init(void *param)
{
    struct image_st *par;
    rt_err_t ret = RT_EOK;

    if (param)
    {
        par = (struct image_st *)param;
        target = *par;
    }
    RT_ASSERT(app_main_data->clock_style < CLOCK_STYLE_MAX_NUM);

    if (clock_init_func[app_main_data->clock_style].cb && (clock_init_func[app_main_data->clock_style].inited == 0))
    {
        ret = clock_init_func[app_main_data->clock_style].cb();
        clock_init_func[app_main_data->clock_style].inited = 1;
    }

    return ret;
}

rt_err_t app_clock_design(void *param)
{
    rt_err_t ret = RT_EOK;

    RT_ASSERT(app_main_data->clock_style < CLOCK_STYLE_MAX_NUM);

    if (clock_design_func[app_main_data->clock_style].cb)
    {
        ret = clock_design_func[app_main_data->clock_style].cb(param);
    }

    return ret;
}
