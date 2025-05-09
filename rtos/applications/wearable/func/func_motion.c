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

extern lv_font_t lv_font_simhei_36;
/*
 **************************************************************************************************
 *
 * initial
 *
 **************************************************************************************************
 */
static lv_style_t main_style;
static lv_style_t arc0_style;
static lv_style_t arc1_style;
static lv_style_t arc2_style;
static lv_style_t list_lable;
static image_info_t list_info[3];

void func_motion_init(void *param)
{
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *pdata = page->private;
    rt_uint16_t i;
    img_load_info_t img_load_info;
    char img_name[64];
    image_info_t *img_info = NULL;

    pdata->alpha_win = 0;
    page->w = MOTION_WIN_FB_W;
    page->h = MOTION_WIN_FB_H;
    page->vir_w = MOTION_WIN_FB_W;
    page->ver_offset = 0;
    page->hor_offset = 0;

    /* framebuffer malloc */
    page->fblen = MOTION_WIN_FB_W * MOTION_WIN_FB_H * MOTION_WIN_COLOR_DEPTH >> 3;
    page->fb   = (rt_uint8_t *)rt_malloc_large(page->fblen);
    RT_ASSERT(page->fb != RT_NULL);
    rt_memset(page->fb, 0, page->fblen);

    //load info
    for (i = 0; i < 3; i++)
    {
        rt_memset(&list_info[i], 0, sizeof(image_info_t));
        list_info[i].type  = IMG_TYPE_RAW;
        list_info[i].pixel = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        list_info[i].x     = 0;
        list_info[i].y     = 0;
        list_info[i].w     = 32;
        list_info[i].h     = 32;
        list_info[i].size  = list_info[i].w * list_info[i].h * 2,
                    list_info[i].data  = (rt_uint8_t *)rt_malloc_large(list_info[i].size);
        RT_ASSERT(list_info[i].data != RT_NULL);
        rt_memset(img_name, 0, sizeof(img_name));
        snprintf(img_name, sizeof(img_name), ICONS_PATH"/icon_sport_%02d.dta", i);
        //rt_kprintf("name = %s\n", img_name);
        img_load_info.w    = list_info[i].w;
        img_load_info.h    = list_info[i].h;
        img_load_info.name = (const char *)&img_name;
        app_load_img(&img_load_info, list_info[i].data, list_info[i].w, list_info[i].h, 0, 2);
    }

    /* Background */
    lv_style_init(&main_style);
    lv_style_set_bg_color(&main_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_radius(&main_style, LV_STATE_DEFAULT, 0/*LV_RADIUS_CIRCLE*/);
    lv_style_set_border_width(&main_style, LV_STATE_DEFAULT, 0);

    lv_obj_t *obj_main = lv_obj_create(lv_scr_act(), NULL);
    lv_obj_add_style(obj_main, LV_STATE_DEFAULT, &main_style);
    lv_obj_set_size(obj_main, MOTION_WIN_FB_W, MOTION_WIN_FB_H);
    lv_obj_set_pos(obj_main, 0, 0);

    /*Arc 0*/
    lv_style_init(&arc0_style);
    lv_style_set_bg_color(&arc0_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_radius(&arc0_style, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&arc0_style, LV_STATE_DEFAULT, 0);
    lv_style_set_line_color(&arc0_style, LV_STATE_DEFAULT, LV_COLOR_BLUE);

    lv_obj_t *arc0 = lv_arc_create(obj_main, NULL);
    lv_obj_add_style(arc0, LV_STATE_DEFAULT, &arc0_style);
    lv_arc_set_angles(arc0, 0, 0);
    lv_arc_set_rotation(arc0, 270);
    lv_arc_set_bg_angles(arc0, 0, 260);
    lv_obj_set_size(arc0, 364, 364);
    lv_obj_align(arc0, obj_main, LV_ALIGN_CENTER, 0, 0);

    /*Arc 1*/
    lv_style_init(&arc1_style);
    lv_style_set_bg_color(&arc1_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_radius(&arc1_style, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&arc1_style, LV_STATE_DEFAULT, 0);
    lv_style_set_line_color(&arc1_style, LV_STATE_DEFAULT, LV_COLOR_GREEN);

    lv_obj_t *arc1 = lv_arc_create(obj_main, NULL);
    lv_obj_add_style(arc1, LV_STATE_DEFAULT, &arc1_style);
    lv_arc_set_angles(arc1, 0, 0);
    lv_arc_set_rotation(arc1, 270);
    lv_arc_set_bg_angles(arc1, 0, 240);
    lv_obj_set_size(arc1, 324, 324);
    lv_obj_align(arc1, obj_main, LV_ALIGN_CENTER, 0, 0);

    /*Arc 2*/
    lv_style_init(&arc2_style);
    lv_style_set_bg_color(&arc2_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_radius(&arc2_style, LV_STATE_DEFAULT, LV_RADIUS_CIRCLE);
    lv_style_set_border_width(&arc2_style, LV_STATE_DEFAULT, 0);
    lv_style_set_line_color(&arc2_style, LV_STATE_DEFAULT, LV_COLOR_ORANGE);

    lv_obj_t *arc2 = lv_arc_create(obj_main, NULL);
    lv_obj_add_style(arc2, LV_STATE_DEFAULT, &arc2_style);
    lv_arc_set_angles(arc2, 0, 0);
    lv_arc_set_rotation(arc2, 270);
    lv_arc_set_bg_angles(arc2, 0, 270);
    lv_obj_set_size(arc2, 284, 284);
    lv_obj_align(arc2, obj_main, LV_ALIGN_CENTER, 0, 0);

    lv_style_init(&list_lable);
    lv_style_set_text_font(&list_lable, LV_STATE_DEFAULT, &lv_font_simhei_36);

    lv_obj_t *label_0 = lv_label_create(obj_main, NULL);
    lv_obj_add_style(label_0, LV_STATE_DEFAULT, &list_lable);
    lv_obj_set_pos(label_0, MOTION_LABEL_X, 155);
    lv_label_set_text(label_0, "4558");

    lv_obj_t *label_1 = lv_label_create(obj_main, NULL);
    lv_obj_add_style(label_1, LV_STATE_DEFAULT, &list_lable);
    lv_obj_set_pos(label_1, MOTION_LABEL_X, 205);
    lv_label_set_text(label_1, "20/30分钟");

    lv_obj_t *label_2 = lv_label_create(obj_main, NULL);
    lv_obj_add_style(label_2, LV_STATE_DEFAULT, &list_lable);
    lv_obj_set_pos(label_2, MOTION_LABEL_X, 255);
    lv_label_set_text(label_2, "9/12");

    //draw all......
    lv_refr_now(lv_disp_get_default());

    struct image_st ps;

    ps.width  = LV_HOR_RES;
    ps.height = LV_VER_RES;
    ps.stride = LV_HOR_RES * (LV_COLOR_DEPTH >> 3);
    ps.pdata = g_lvdata->fb;

    if (param == NULL)
    {
        struct image_st pd;

        pd.width  = MOTION_WIN_XRES;
        pd.height = MOTION_WIN_YRES;
        pd.stride = MOTION_WIN_XRES * (MOTION_WIN_COLOR_DEPTH >> 3);
        pd.pdata = page->fb;

        rk_image_copy(&ps, &pd, LV_COLOR_DEPTH >> 3);

        rt_uint16_t startx;
        rt_uint16_t starty;
        for (i = 0; i < 3; i++)
        {
            startx = MOTION_ICONS_X;
            starty = 155 + i * 50;
            img_info = &list_info[i];
            rt_display_img_fill(img_info, page->fb, MOTION_WIN_FB_W, startx, starty);
        }
    }
    else
    {
        struct image_st *pd = (struct image_st *)param;

        rk_image_copy(&ps, pd, LV_COLOR_DEPTH >> 3);

        pd->pdata += (MOTION_ICONS_X * (MOTION_WIN_COLOR_DEPTH >> 3) + 155 * pd->stride);
        for (i = 0; i < 3; i++)
        {
            img_info = &list_info[i];
            ps.width  = img_info->w;
            ps.height = img_info->h;
            ps.stride = img_info->size / img_info->h;
            ps.pdata = img_info->data;

            rk_image_copy(&ps, pd, LV_COLOR_DEPTH >> 3);
            pd->pdata += 50 * pd->stride;
        }
    }

    lv_obj_del(obj_main);

    app_func_set_preview(APP_FUNC_EXERCISE, page->fb);
}

void func_motion_exit(void)
{
    struct app_page_data_t *page = g_func_page;
    rt_free_large(page->fb);

    for (rt_uint16_t i = 0; i < 3; i++)
    {
        rt_free_large(list_info[i].data);
    }
}

void func_motion_enter(void *param)
{
    app_func_show(param);
}

struct app_func func_motion_ops =
{
    .init = func_motion_init,
    .enter = func_motion_enter,
    .exit = func_motion_exit,
};
