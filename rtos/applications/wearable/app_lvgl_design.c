/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#include <math.h>

#include <dfs_posix.h>
#include "drv_heap.h"

#include "drv_display.h"
#include "image_info.h"
#include "display.h"

#include "app_main.h"

#define LV_FB_W             LV_HOR_RES
#define LV_FB_H             LV_VER_RES
#define LV_FB_COLOR_DEPTH   LV_COLOR_DEPTH

#define SHOW_TICK           0
#define LVGL_FONT_DESIGN    1

#define IMG_USING_DEFAULT_HEAP  0

#if !LVGL_FONT_DESIGN
#include "rk_lv_font.h"
#endif

struct app_lvgl_design_data_t *g_lvdata = RT_NULL;
static int lvgl_init_done = 0;
/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
/**
 * hook function for lvgl set graphic info.
 */
rt_err_t lv_set_graphic_info(struct rt_device_graphic_info *info)
{
    struct display_state *state = (struct display_state *)app_main_data->disp->device->user_data;
    struct rt_device_graphic_info *graphic_info = &state->graphic_info;

    graphic_info->bits_per_pixel = LV_FB_COLOR_DEPTH;
    graphic_info->width          = LV_FB_W;
    graphic_info->height         = LV_FB_H;
    graphic_info->framebuffer    = g_lvdata->fb;

    memcpy(info, graphic_info, sizeof(struct rt_device_graphic_info));

    return RT_EOK;
}

rt_err_t lv_clock_img_file_load(lv_img_dsc_t *img_dsc, const char *file)
{
    lv_res_t res;

    //rt_kprintf("name = %s\n", file);

    lv_img_header_t info;
    res = lv_img_decoder_get_info(file, &info);
    if (res != LV_RES_OK)
    {
        rt_kprintf("%s open failed\n", file);
        img_dsc->data = NULL;
        return -RT_ERROR;
    }

    uint8_t px_size = lv_img_cf_get_px_size(info.cf);
    RT_ASSERT(px_size == LV_COLOR_DEPTH);

    //rt_kprintf("%s: %d, %d, %d, %d\n", file, info.w, info.h, px_size, info.w * info.h * px_size / 8);

    uint8_t *img_buf;
#if IMG_USING_DEFAULT_HEAP
    if (RT_PSRAM_MALLOC_THRRESH > info.w * info.h * px_size / 8)
    {
        img_buf = rt_malloc(info.w * info.h * px_size / 8);
    }
    else
#endif
    {
        img_buf = rt_malloc_psram(info.w * info.h * px_size / 8);
    }
    RT_ASSERT(img_buf != RT_NULL);
    rt_memset((void *)img_buf, 0, info.w * info.h * px_size / 8);

    lv_img_decoder_dsc_t dsc;
    res = lv_img_decoder_open(&dsc, file, LV_COLOR_WHITE);
    RT_ASSERT(res == LV_RES_OK);

    res = lv_img_decoder_read_line(&dsc, 0, 0, info.w * info.h, img_buf);
    //RT_ASSERT(res == LV_RES_OK);

    lv_img_decoder_close(&dsc);

    img_dsc->header.cf = info.cf;
    img_dsc->header.always_zero = info.always_zero;
    img_dsc->header.w = info.w;
    img_dsc->header.h = info.h;
    img_dsc->data_size = info.w * info.h * px_size / 8;
    img_dsc->data = img_buf;

    return RT_EOK;
}

void lv_clock_img_dsc_free(lv_img_dsc_t *img_dsc)
{
    if (img_dsc->data == NULL)
        return;
#if IMG_USING_DEFAULT_HEAP
    if (RT_PSRAM_MALLOC_THRRESH > img_dsc->data_size)
    {
        rt_free((void *)img_dsc->data);
    }
    else
#endif
    {
        rt_free_psram((void *)img_dsc->data);
    }
}

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern struct app_lvgl_label_design g_name;

static lv_style_t main_style;
static lv_style_t label_style;
static lv_obj_t *obj_main = NULL;

int funclist_x_pos[ARRAY_MAX_ICONS];
int funclist_y_pos[ARRAY_MAX_ICONS];
int app_lv_icon_index_check(int x_pos, int y_pos)
{
    int start_x, start_y, finish_x, finish_y;
    for (int i = 0; i < FUNC_ICON_HOR_NUM; i++)
    {
        for (int j = 0; j < FUNC_ICON_VER_NUM; j++)
        {
            start_x = funclist_x_pos[i * FUNC_ICON_VER_NUM + j];
            finish_x = start_x + FUNC_ICON_W;
            start_y = funclist_y_pos[i * FUNC_ICON_VER_NUM + j];
            finish_y = start_y + FUNC_ICON_H;

            if ((x_pos >= start_x) && (x_pos <= finish_x) &&
                    (y_pos >= start_y) && (y_pos <= finish_y))
            {
                return (i * FUNC_ICON_VER_NUM + j);
            }
        }
    }

    return -RT_ERROR;
}

static void app_lv_icon_pos_design(int *x_pos, int *y_pos, int hor, int ver, int icon_w, int icon_h)
{
    int top_padding;
    int left_padding;
    int hor_padding = 0, ver_padding = 0;

    if (hor > 1)
    {
        hor_padding = ((DISP_XRES / hor) - icon_w) / 2;
    }
    if (ver > 1)
    {
        ver_padding = (int)((uint32_t)(DISP_YRES - (icon_h * ver)) / (ver + 1)) & ~((ver - 1) & 1UL);
    }

    left_padding = hor_padding;
    top_padding = (DISP_YRES - ver_padding * (ver - 1) - icon_h * ver) / 2;

    for (int i = 0; i < hor; i++)
    {
        for (int j = 0; j < ver; j++)
        {
            x_pos[i * ver + j] = left_padding + i * (icon_w + hor_padding * 2);
            y_pos[i * ver + j] = top_padding + j * (icon_h + ver_padding);
        }
    }
}

rt_err_t app_lv_iconarray_design(void *param)
{
    struct app_lvgl_iconarray_design *design = (struct app_lvgl_iconarray_design *)param;
    int icons_num = MIN(design->icons_num, ARRAY_MAX_ICONS);
    lv_img_dsc_t icon_img;
    struct image_st ps, pd;

    for (int i = 0; i < icons_num; i++)
    {
        if (lv_clock_img_file_load(&icon_img, design->path[i]) == -RT_ERROR)
            continue;

        ps.width  = FUNC_ICON_W;
        ps.height = FUNC_ICON_H;
        ps.stride = ps.width * (LV_FB_COLOR_DEPTH >> 3);
        ps.pdata = (unsigned char *)icon_img.data;

        pd.width  = ps.width;
        pd.height = ps.height;
        pd.stride = design->img[0].stride;
        pd.pdata = design->img[0].pdata + funclist_x_pos[i] * (LV_FB_COLOR_DEPTH >> 3) + funclist_y_pos[i] * pd.stride;

        rk_image_copy(&ps, &pd, LV_FB_COLOR_DEPTH >> 3);
        if (design->ping_pong)
        {
            pd.stride = design->img[1].stride;
            pd.pdata = design->img[1].pdata + funclist_x_pos[i] * (LV_FB_COLOR_DEPTH >> 3) + funclist_y_pos[i] * pd.stride;
            rk_image_copy(&ps, &pd, LV_FB_COLOR_DEPTH >> 3);
        }

        lv_clock_img_dsc_free(&icon_img);
    }

    return RT_EOK;
}
design_cb_t lv_lvgl_iconarray_design_t = {.cb = app_lv_iconarray_design,};

static void app_lv_icon_y_design(int *y_pos, int ver, int icon_h)
{
    int top_padding;
    int ver_padding = 0;

    if (ver > 1)
        ver_padding = (int)((uint32_t)(DISP_YRES - (icon_h * ver)) / (ver + 1)) & ~((ver - 1) & 1UL);

    top_padding = (DISP_YRES - ver_padding * (ver - 1) - icon_h * ver) / 2;

    for (int i = 0; i < ver; i++)
    {
        y_pos[i] = top_padding + i * (icon_h + ver_padding);
    }
}

rt_err_t app_lv_iconlist_design(void *param)
{
    struct app_lvgl_iconlist_design *design = (struct app_lvgl_iconlist_design *)param;
    int icons_num = MIN(design->icons_num, LIST_MAX_ICONS);
    lv_img_dsc_t icon_img[LIST_MAX_ICONS];
    lv_img_dsc_t *img_dsc;
    struct image_st ps, pd;
    lv_style_t list_btn_style, main_style;
    lv_obj_t *obj_list;
    lv_obj_t *list_btn[LIST_MAX_ICONS];

    int y_pos[LIST_MAX_ICONS];

    app_lv_icon_y_design(y_pos, FUNC_ICON_VER_NUM, FUNC_ICON_H);

    lv_style_init(&main_style);
    lv_style_set_radius(&main_style, LV_STATE_DEFAULT, 0/*LV_RADIUS_CIRCLE*/);
    lv_style_set_bg_color(&main_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_border_width(&main_style, LV_STATE_DEFAULT, 0);

    lv_style_init(&list_btn_style);
    lv_style_set_bg_opa(&list_btn_style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_bg_color(&list_btn_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_color(&list_btn_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&list_btn_style, LV_STATE_DEFAULT, &lv_font_montserrat_44);
    lv_style_set_pad_top(&list_btn_style, LV_STATE_DEFAULT, 10);
    lv_style_set_pad_bottom(&list_btn_style, LV_STATE_DEFAULT, 10);
    lv_style_set_pad_left(&list_btn_style, LV_STATE_DEFAULT, 0);
    lv_style_set_pad_right(&list_btn_style, LV_STATE_DEFAULT, 0);

    obj_list = lv_list_create(lv_scr_act(), NULL);
    lv_obj_add_style(obj_list, LV_STATE_DEFAULT, &main_style);
    lv_obj_set_size(obj_list, DISP_XRES, DISP_YRES);
    lv_obj_set_pos(obj_list, 0, 0);
    lv_list_set_scrollbar_mode(obj_list, LV_SCROLLBAR_MODE_OFF);

    for (int i = 0; i < icons_num; i++)
    {
        if (design->path[i] != NULL)
        {
            if (lv_clock_img_file_load(&icon_img[i], design->path[i]) == -RT_ERROR)
                continue;
            img_dsc = &icon_img[i];
        }
        else
        {
            img_dsc = NULL;
        }
        list_btn[i] = lv_list_add_btn(obj_list, img_dsc, design->txt[i]);
        lv_obj_add_style(list_btn[i], LV_STATE_DEFAULT, &list_btn_style);
    }

    lv_refr_now(lv_disp_get_default());

    ps.width  = LV_FB_W;
    ps.height = LV_FB_H;
    ps.stride = ps.width * (LV_FB_COLOR_DEPTH >> 3);
    ps.pdata = g_lvdata->fb;

    pd.width  = ps.width;
    pd.height = ps.height;
    pd.stride = design->img[0].stride;
    pd.pdata = design->img[0].pdata;

    rk_image_copy(&ps, &pd, LV_FB_COLOR_DEPTH >> 3);
    if (design->ping_pong)
    {
        pd.stride = design->img[1].stride;
        pd.pdata = design->img[1].pdata;
        rk_image_copy(&ps, &pd, LV_FB_COLOR_DEPTH >> 3);
    }

    for (int i = 0; i < icons_num; i++)
    {
        if (design->path[i] != NULL)
        {
            lv_clock_img_dsc_free(&icon_img[i]);
        }
    }

    lv_obj_del(obj_list);

    return RT_EOK;
}
design_cb_t lv_lvgl_iconlist_design_t = {.cb = app_lv_iconlist_design,};

rt_err_t app_lv_label_design(void *param)
{
#if LVGL_FONT_DESIGN
    struct app_lvgl_label_design *design = (struct app_lvgl_label_design *)param;
    struct image_st ps;

    if (design->img[0].width == 0 ||
            design->img[0].height == 0 ||
            design->img[0].pdata == NULL ||
            (design->ping_pong && design->img[1].pdata == NULL) ||
            design->txt == NULL ||
            obj_main == NULL)
    {
        rt_kprintf("%s error\n", __func__);
        return -RT_ERROR;
    }

    design->img[0].width = MIN(design->img[0].width, LV_FB_W);
    design->img[0].height = MIN(design->img[0].height, LV_FB_H);
    if (design->ping_pong)
    {
        design->img[1].width = design->img[0].width;
        design->img[1].height = design->img[0].height;
    }
    if (design->font)
    {
        lv_style_set_text_font(&label_style, LV_STATE_DEFAULT, design->font);
    }
    else
    {
        lv_style_set_text_font(&label_style, LV_STATE_DEFAULT, &lv_font_montserrat_30);
    }

    lv_obj_t *label = lv_label_create(obj_main, NULL);
    lv_obj_add_style(label, LV_STATE_DEFAULT, &label_style);
    lv_obj_set_pos(label, 0, 0);

    lv_label_set_long_mode(label, LV_LABEL_LONG_BREAK);
    lv_label_set_align(label, design->align);
    lv_obj_set_size(label, design->img[0].width, design->img[0].height);
    lv_label_set_text(label, (const char *)design->txt);

    ps.width  = lv_obj_get_width(label);
    ps.height = lv_obj_get_height(label);
    ps.stride = LV_FB_W * (LV_FB_COLOR_DEPTH >> 3);
    ps.pdata  = g_lvdata->fb;

    lv_refr_now(lv_disp_get_default());

    if (design->fmt == RTGRAPHIC_PIXEL_FORMAT_RGB888)
    {
        rk_image_copy_to888(&ps, &design->img[0], LV_FB_COLOR_DEPTH >> 3);
        if (design->ping_pong)
        {
            rk_image_copy_to888(&ps, &design->img[1], LV_FB_COLOR_DEPTH >> 3);
        }
    }
    else
    {
        rk_image_copy(&ps, &design->img[0], LV_FB_COLOR_DEPTH >> 3);
        if (design->ping_pong)
        {
            rk_image_copy(&ps, &design->img[1], LV_FB_COLOR_DEPTH >> 3);
        }
    }

    rk_image_reset(&ps, LV_FB_COLOR_DEPTH >> 3);

    lv_obj_del(label);
#else
    struct app_lvgl_label_design *design = (struct app_lvgl_label_design *)param;
    rk_lv_label_t label;

    label.dst = design->img[0];
    label.font = design->font;
    label.align = design->align;
    label.fcolor.r = 0xFF;
    label.fcolor.g = 0xFF;
    label.fcolor.b = 0xFF;
    label.bcolor.r = 0x00;
    label.bcolor.g = 0x00;
    label.bcolor.b = 0x00;
    label.line_pad = 3;
    label.letter_pad = 2;
    label.space_pad = 5;

    rk_image_reset(&label.dst, LV_FB_COLOR_DEPTH >> 3);
    rk_lv_draw_txt(&label, (uint8_t *)design->txt);

    if (design->ping_pong)
    {
        rk_image_copy(&design->img[0], &design->img[1], LV_FB_COLOR_DEPTH >> 3);
    }

#endif

    return RT_EOK;
}
design_cb_t lv_lvgl_label_design_t = {.cb = app_lv_label_design,};

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
static rt_err_t app_lvgl_init_design(void *param)
{
    struct app_lvgl_design_data_t *pdata;
    rt_err_t ret;

    g_lvdata = pdata = (struct app_lvgl_design_data_t *)rt_malloc(sizeof(struct app_lvgl_design_data_t));
    RT_ASSERT(pdata != RT_NULL);
    rt_memset((void *)pdata, 0, sizeof(struct app_lvgl_design_data_t));

    /* framebuffer malloc */
    pdata->fblen = LV_FB_W * LV_FB_H * LV_FB_COLOR_DEPTH / 8;
    pdata->fb   = (rt_uint8_t *)rt_malloc_psram(pdata->fblen);
    RT_ASSERT(pdata->fb != RT_NULL);

    ret = littlevgl2rtt_init("lcd");
    RT_ASSERT(ret == RT_EOK);

    /* style init */
    lv_style_init(&main_style);
    lv_style_set_bg_color(&main_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_radius(&main_style, LV_STATE_DEFAULT, 0/*LV_RADIUS_CIRCLE*/);
    lv_style_set_border_width(&main_style, LV_STATE_DEFAULT, 0);

    lv_style_init(&label_style);
    lv_style_set_bg_opa(&label_style, LV_STATE_DEFAULT, LV_OPA_COVER);
    lv_style_set_bg_color(&label_style, LV_STATE_DEFAULT, LV_COLOR_BLACK);
    lv_style_set_text_color(&label_style, LV_STATE_DEFAULT, LV_COLOR_WHITE);
    lv_style_set_text_font(&label_style, LV_STATE_DEFAULT, &lv_font_montserrat_30);

    /* Background */
    if (obj_main == NULL)
    {
        obj_main = lv_obj_create(lv_scr_act(), NULL);
    }
    lv_obj_add_style(obj_main, LV_STATE_DEFAULT, &main_style);
    lv_obj_set_size(obj_main, LV_HOR_RES, LV_VER_RES);
    lv_obj_set_pos(obj_main, 0, 0);

    app_lv_icon_pos_design(funclist_x_pos, funclist_y_pos, FUNC_ICON_HOR_NUM, FUNC_ICON_VER_NUM, FUNC_ICON_W, FUNC_ICON_H);

    lvgl_init_done = 1;

    return RT_EOK;
}
// static design_cb_t lvgl_init_design_t = {.cb = app_lvgl_init_design,};

int app_lvgl_init_check(void)
{
    return lvgl_init_done;
}

void app_lvgl_init(void)
{
    app_lvgl_init_design(RT_NULL);
    // app_design_request(0, &lvgl_init_design_t, RT_NULL);
}
