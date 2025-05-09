/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#ifndef __APP_LVGL_DESIGN__
#define __APP_LVGL_DESIGN__
#include <rtthread.h>
#include "lib_imageprocess.h"

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */

#define ARRAY_MAX_ICONS     6
#define LIST_MAX_ICONS      3

struct app_lvgl_design_data_t
{
    rt_uint8_t *fb;
    rt_uint32_t fblen;
};
extern struct app_lvgl_design_data_t *g_lvdata;

struct app_lvgl_label_design
{
    char *txt;
    void *font;
    rt_int8_t align;
    rt_uint32_t fmt;
    struct image_st img[2];
    rt_uint8_t ping_pong;
};

struct app_lvgl_iconarray_design
{
    int icons_num;
    char *path[ARRAY_MAX_ICONS];
    struct image_st img[2];
    rt_uint8_t ping_pong;
};

struct app_lvgl_iconlist_design
{
    int icons_num;
    char *txt[LIST_MAX_ICONS];
    char *path[LIST_MAX_ICONS];
    struct image_st img[2];
    rt_uint8_t ping_pong;
};

/*
 **************************************************************************************************
 *
 * Struct & data define
 *
 **************************************************************************************************
 */
extern design_cb_t lv_lvgl_label_design_t;
extern design_cb_t lv_lvgl_iconarray_design_t;

void app_lvgl_init(void);
int app_lvgl_init_check(void);
rt_err_t app_lv_label_design(void *param);
rt_err_t app_lv_iconarray_design(void *param);
rt_err_t app_lv_iconlist_design(void *param);
int app_lv_icon_index_check(int x_pos, int y_pos);
rt_err_t lv_clock_img_file_load(lv_img_dsc_t *img_dsc, const char *file);

#endif
