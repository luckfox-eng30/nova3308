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
/*
 **************************************************************************************************
 *
 * Music design callback
 *
 **************************************************************************************************
 */
static player_state_t play_state;
static rt_int8_t play_mode;
static rt_int8_t play_vol;
static int first_load = 1;
static struct image_st target;

static const char *app_play_vol_icons[5] =
{
    MUSIC_ICONS_PATH"/icon_music_mute.dta",
    MUSIC_ICONS_PATH"/icon_music_vol_25.dta",
    MUSIC_ICONS_PATH"/icon_music_vol_50.dta",
    MUSIC_ICONS_PATH"/icon_music_vol_75.dta",
    MUSIC_ICONS_PATH"/icon_music_vol_100.dta",
};

rt_err_t app_music_init(void *param)
{
    struct image_st *par = (struct image_st *)param;

    target = *par;

    return RT_EOK;
}

rt_err_t app_music_design(void *param)
{
    struct image_st *par = (struct image_st *)&target;
    rt_uint8_t  *fb   = par->pdata;
    rt_uint32_t vir_w = par->stride / format2depth[par->format];
    img_load_info_t img_load_info;
    rt_uint16_t startx = 0;
    rt_uint16_t starty = 0;
    rt_uint8_t  *buf;

    if (first_load)
    {
        rk_image_reset(par, format2depth[par->format]);

        startx = MUSIC_MORE_X;
        starty = MUSIC_MORE_Y;
        buf = fb + (startx + starty * vir_w) * format2depth[par->format];
        img_load_info.w = MUSIC_MORE_W;
        img_load_info.h = MUSIC_MORE_H;
        img_load_info.name = MUSIC_ICONS_PATH"/icon_nusic_more.dta";
        app_load_img(&img_load_info, buf, vir_w, img_load_info.h, 0, format2depth[par->format]);

        startx = MUSIC_PREV_X;
        starty = MUSIC_PREV_Y;
        buf = fb + (startx + starty * vir_w) * format2depth[par->format];
        img_load_info.w = MUSIC_PREV_W;
        img_load_info.h = MUSIC_PREV_H;
        img_load_info.name = MUSIC_ICONS_PATH"/icon_nusic_prev.dta";
        app_load_img(&img_load_info, buf, vir_w, img_load_info.h, 0, format2depth[par->format]);

        startx = MUSIC_NEXT_X;
        starty = MUSIC_NEXT_Y;
        buf = fb + (startx + starty * vir_w) * format2depth[par->format];
        img_load_info.w = MUSIC_NEXT_W;
        img_load_info.h = MUSIC_NEXT_H;
        img_load_info.name = MUSIC_ICONS_PATH"/icon_nusic_next.dta";
        app_load_img(&img_load_info, buf, vir_w, img_load_info.h, 0, format2depth[par->format]);

        startx = MUSIC_PIC_X;
        starty = MUSIC_PIC_Y;
        buf = fb + (startx + starty * vir_w) * format2depth[par->format];
        img_load_info.w = MUSIC_PIC_W;
        img_load_info.h = MUSIC_PIC_H;
        img_load_info.name = MUSIC_ICONS_PATH"/music_picture.dta";
        app_load_img(&img_load_info, buf, vir_w, img_load_info.h, 0, format2depth[par->format]);
    }

    if ((play_state != app_main_data->play_state) || first_load)
    {
        play_state = app_main_data->play_state;

        switch (play_state)
        {
        case PLAYER_STATE_RUNNING:
            img_load_info.name = MUSIC_ICONS_PATH"/icon_music_suspend.dta";
            break;
        default:
            img_load_info.name = MUSIC_ICONS_PATH"/icon_nusic_play.dta";
            break;
        }
        startx = MUSIC_PLAY_X;
        starty = MUSIC_PLAY_Y;
        buf = fb + (startx + starty * vir_w) * format2depth[par->format];
        img_load_info.w = MUSIC_PLAY_W;
        img_load_info.h = MUSIC_PLAY_H;
        app_load_img(&img_load_info, buf, vir_w, img_load_info.h, 0, format2depth[par->format]);
    }

    if ((play_mode != app_main_data->play_mode) || first_load)
    {
        char txt[128];

        if (app_main_data->play_mode < APP_PLAY_LIST)
            app_main_data->play_mode = APP_PLAY_LIST;
        if (app_main_data->play_mode > APP_PLAY_RANDOM)
            app_main_data->play_mode = APP_PLAY_RANDOM;

        play_mode = app_main_data->play_mode;

        memset(txt, 0, sizeof(txt));
        snprintf(txt, sizeof(txt), MUSIC_ICONS_PATH"/icon_music_%02d.dta", play_mode);

        startx = MUSIC_MODE_X;
        starty = MUSIC_MODE_Y;
        buf = fb + (startx + starty * vir_w) * format2depth[par->format];
        img_load_info.w = MUSIC_MODE_W;
        img_load_info.h = MUSIC_MODE_H;
        img_load_info.name = txt;
        app_load_img(&img_load_info, buf, vir_w, img_load_info.h, 0, format2depth[par->format]);
    }

    if ((play_vol != app_main_data->play_vol) || first_load)
    {
        if (app_main_data->play_vol < APP_PLAY_VOL_MIN)
            app_main_data->play_vol = APP_PLAY_VOL_MIN;
        if (app_main_data->play_vol > APP_PLAY_VOL_MAX)
            app_main_data->play_vol = APP_PLAY_VOL_MAX;

        play_vol = app_main_data->play_vol;

        startx = MUSIC_VOL_X;
        starty = MUSIC_VOL_Y;
        buf = fb + (startx + starty * vir_w) * format2depth[par->format];
        img_load_info.w = MUSIC_VOL_W;
        img_load_info.h = MUSIC_VOL_H;
        img_load_info.name = app_play_vol_icons[play_vol];
        app_load_img(&img_load_info, buf, vir_w, img_load_info.h, 0, format2depth[par->format]);
    }

    first_load = 0;

    return RT_EOK;
}

static int rotate_angle = 0;
void app_music_name_design(char *name)
{
    struct image_st *par = (struct image_st *)&target;
    rt_uint8_t  *fb   = par->pdata;
    rt_uint32_t vir_w = par->stride / format2depth[par->format];
    struct app_lvgl_label_design label;
    int start_x, start_y;

    start_x = 0;
    start_y = MUSIC_NAME_Y;
    if (name)
    {
        label.txt = name;
    }
    else
    {
        label.txt = "No File";
    }

    label.ping_pong = 0;
    label.font = &lv_font_montserrat_30;
    label.align = LV_LABEL_ALIGN_CENTER;
    label.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    label.img[0].width = MUSIC_NAME_W;
    label.img[0].height = lv_font_montserrat_30.line_height;
    label.img[0].stride = vir_w * format2depth[par->format];
    label.img[0].pdata = fb + (start_x + start_y * vir_w) * format2depth[par->format];
    app_lv_label_design(&label);
    rotate_angle = 0;
}

static page_refrsh_request_param_t app_music_refr_param;
void app_music_design_update(void)
{
    app_music_design(&target);

    if (app_main_data->ver_page == VER_PAGE_NULL)
    {
        app_music_refr_param.page = app_main_page;
        app_music_refr_param.page_num = 1;
        app_refresh_request(&app_music_refr_param);
    }
}

struct rotateimage_st r_ps = {0, 0, 0, 0, 0, NULL};
struct rotateimage_st r_pd = {0, 0, 0, 0, 0, NULL};
rt_err_t app_music_picture_rotate(void *param)
{
    struct image_st *par = (struct image_st *)&target;
    rt_uint8_t  *fb   = par->pdata;
    rt_uint32_t vir_w = par->stride / format2depth[par->format];
    uint16_t start_x = MUSIC_PIC_X;
    uint16_t start_y = MUSIC_PIC_Y;
    img_load_info_t img_load_info;

    if (app_main_data->ver_page != VER_PAGE_NULL)
    {
        return RT_EOK;
    }

    if (r_ps.pdata == NULL)
    {
        r_ps.width = MUSIC_PIC_W;
        r_ps.height = MUSIC_PIC_H;
        r_ps.cx = r_ps.width / 2 - 0.5;
        r_ps.cy = r_ps.height / 2 - 0.5;
        r_ps.stride = r_ps.width * 4;
        r_ps.pdata = rt_malloc_large(r_ps.height * r_ps.stride);
        if (r_ps.pdata == NULL)
        {
            rt_kprintf("Malloc %d failed\n", r_ps.height * r_ps.stride);
            return -RT_ERROR;
        }
        img_load_info.w = MUSIC_PIC_W;
        img_load_info.h = MUSIC_PIC_H;
        img_load_info.name = MUSIC_ICONS_PATH"/music_picture_argb.dta";
        app_load_img(&img_load_info, r_ps.pdata, r_ps.width, r_ps.height, 0, 4);
    }

    if (r_pd.pdata == NULL)
    {
        r_pd.width = MUSIC_PIC_W;
        r_pd.height = MUSIC_PIC_H;
        r_pd.cx = r_pd.width / 2 - 0.5;
        r_pd.cy = r_pd.height / 2 - 0.5;
        r_pd.stride = vir_w * format2depth[par->format];
        r_pd.pdata = fb + (start_x + start_y * vir_w) * format2depth[par->format];
    }

    rotate_angle += 10;
    if (rotate_angle >= 360)
        rotate_angle = 0;
    rk_rotate_process_16bit(&r_ps, &r_pd, 360 - (rotate_angle % 360));

    app_music_refr_param.page = app_main_page;
    app_music_refr_param.page_num = 1;
    app_refresh_request(&app_music_refr_param);

    return RT_EOK;
}

void app_music_page_leave(void)
{
    rt_free_large(r_ps.pdata);
    r_ps.pdata = NULL;
    r_pd.pdata = NULL;
}

struct obj_area
{
    uint16_t x;
    uint16_t y;
    uint16_t w;
    uint16_t h;
    void (*cb)(void);
};

struct obj_area btn[6] =
{
    {MUSIC_MODE_X, MUSIC_MODE_Y, MUSIC_MODE_W, MUSIC_MODE_H, app_music_mode_switch},
    {MUSIC_VOL_X,  MUSIC_VOL_Y,  MUSIC_VOL_W,  MUSIC_VOL_H, app_music_vol_switch},
    {MUSIC_MORE_X, MUSIC_MORE_Y, MUSIC_MORE_W, MUSIC_MORE_H, NULL},

    {MUSIC_PREV_TOUCH_X, MUSIC_PREV_TOUCH_Y, MUSIC_PREV_TOUCH_W, MUSIC_PREV_TOUCH_H, app_play_prev},
    {MUSIC_PLAY_X, MUSIC_PLAY_Y, MUSIC_PLAY_W, MUSIC_PLAY_H, app_play_pause},
    {MUSIC_NEXT_TOUCH_X, MUSIC_NEXT_TOUCH_Y, MUSIC_NEXT_TOUCH_W, MUSIC_NEXT_TOUCH_H, app_play_next},
};

void app_music_touch(struct rt_touch_data *point)
{
    uint16_t x = point->x_coordinate;
    uint16_t y = point->y_coordinate;

    for (int i = 0; i < sizeof(btn) / sizeof(struct obj_area); i++)
    {
        if ((x >= btn[i].x) && (x <= (btn[i].x + btn[i].w)) &&
                (y >= btn[i].y) && (y <= (btn[i].y + btn[i].h)))
        {
            if (btn[i].cb != NULL)
                btn[i].cb();
            break;
        }
    }
}
