/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include "app_main.h"

#define HEART_RATE_MAX  180
#define HEART_RATE_MIN  60

static int scale_val_grp[] = {32, 24, 20, 16, 12, 8, 4, 0, 4, 8, 12, 16, 20, 24};
static int scale_index = 0;

static int g_rate[24] = {0};
static int g_rate_cnt = 0;
static int g_rate_min = 0xFFFF;
static int g_rate_max = -1;

static int g_anim_delay;

void plot_shadow(uint8_t *fb, int x, int y, float c)
{
    uint8_t R = ((uint8_t)((217 - 73) * c) + 73);
    uint8_t G = ((uint8_t)((46  - 20) * c) + 20);
    uint8_t B = ((uint8_t)((126 - 42) * c) + 42);

    uint8_t *buf = (fb + (x + (HEARTRATE_CHART_H - y) * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3));
    if (buf[0] == 0 && buf[1] == 0)
    {
        buf[0] = ((G << 3) & 0xe0) | (B >> 3);
        buf[1] = (R & 0xf8) | (G >> 5);
    }
    else if (buf[0] == 0xFF && buf[1] == 0xFF)
    {
        R = R * 0.5 + 255 * 0.5;
        G = G * 0.5 + 255 * 0.5;
        B = B * 0.5 + 255 * 0.5;
        buf[0] = ((G << 3) & 0xe0) | (B >> 3);
        buf[1] = (R & 0xf8) | (G >> 5);
    }
    // if (buf[0] == 0 && buf[1] == 0 && buf[2] == 0)
    // {
    //     *buf++ = B;
    //     *buf++ = G;
    //     *buf = R;
    // }
}

void plot(uint8_t *fb, int x, int y, float c)
{
    uint8_t R = ((uint8_t)((217 - 73) * c) + 73);
    uint8_t G = ((uint8_t)((46  - 20) * c) + 20);
    uint8_t B = ((uint8_t)((126 - 42) * c) + 42);

    uint8_t *buf = (fb + (x + (HEARTRATE_CHART_H - y) * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3));
    buf[0] = ((G << 3) & 0xe0) | (B >> 3);
    buf[1] = (R & 0xf8) | (G >> 5);
    // *buf++ = B;
    // *buf++ = G;
    // *buf = R;
}

static void design_now_value(int val)
{
    struct app_page_data_t *page = g_func_page;
    struct app_lvgl_label_design value;
    int start_x, start_y;
    char txt[128];

    start_x = HEARTRATE_ICON_X + img_heart_info.w + 10;
    start_y = HEARTRATE_RISE_ICON_Y - 44 - 20;
    memset(txt, 0x0, sizeof(txt));
    snprintf(txt, sizeof(txt), "Now %3d", val);
    value.txt = txt;
    value.ping_pong = 0;
    value.font = &lv_font_montserrat_44;
    value.align = LV_LABEL_ALIGN_LEFT;
    value.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    value.img[0].width = MENU_WIN_XRES - start_x;
    value.img[0].height = lv_font_montserrat_44.line_height;
    value.img[0].stride = MENU_WIN_FB_W * (MENU_WIN_COLOR_DEPTH >> 3);
    value.img[0].pdata = page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3);
    app_lv_label_design(&value);
}

static void design_value_range(int max, int min)
{
    struct app_page_data_t *page = g_func_page;
    struct app_lvgl_label_design value;
    img_load_info_t img_load_info;
    rt_uint8_t *buf;
    int start_x, start_y;
    char txt[128];

    start_x = HEARTRATE_RISE_ICON_X + HEARTRATE_RISE_ICON_W;
    start_y = HEARTRATE_RISE_ICON_Y;
    memset(txt, 0x0, sizeof(txt));

    snprintf(txt, sizeof(txt), "%3d", max);
    value.txt = txt;
    value.ping_pong = 0;
    value.font = &lv_font_montserrat_30;
    value.align = LV_LABEL_ALIGN_LEFT;
    value.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    value.img[0].width = MENU_WIN_XRES - start_x;
    value.img[0].height = lv_font_montserrat_30.line_height;
    value.img[0].stride = MENU_WIN_FB_W * (MENU_WIN_COLOR_DEPTH >> 3);
    value.img[0].pdata = page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3);
    app_lv_label_design(&value);

    start_x = HEARTRATE_FALL_ICON_X + HEARTRATE_FALL_ICON_W;
    memset(txt, 0x0, sizeof(txt));

    snprintf(txt, sizeof(txt), "%3d", min);
    value.img[0].width = MENU_WIN_XRES - start_x;
    value.img[0].pdata = page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3);
    app_lv_label_design(&value);

    start_x = HEARTRATE_RISE_ICON_X;
    start_y = HEARTRATE_RISE_ICON_Y;
    buf = page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3);
    img_load_info.w = HEARTRATE_RISE_ICON_W;
    img_load_info.h = HEARTRATE_RISE_ICON_H;
    img_load_info.name = ICONS_PATH"/icon_heart_rise.dta";
    app_load_img(&img_load_info, buf, MENU_WIN_XRES, HEARTRATE_RISE_ICON_H, 0, (MENU_WIN_COLOR_DEPTH >> 3));

    start_x = HEARTRATE_FALL_ICON_X;
    start_y = HEARTRATE_FALL_ICON_Y;
    buf = page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3);
    img_load_info.w = HEARTRATE_FALL_ICON_W;
    img_load_info.h = HEARTRATE_FALL_ICON_H;
    img_load_info.name = ICONS_PATH"/icon_heart_fall.dta";
    app_load_img(&img_load_info, buf, MENU_WIN_XRES, HEARTRATE_FALL_ICON_H, 0, (MENU_WIN_COLOR_DEPTH >> 3));
}

static void design_form(void)
{
    struct app_page_data_t *page = g_func_page;
    struct app_lvgl_label_design value;
    int start_x, start_y;
    int line_start_x, line_start_y;
    uint16_t *buf;
    int val;
    char txt[128];

    start_x = HEARTRATE_CHART_X + HEARTRATE_CHART_W;
    start_y = HEARTRATE_CHART_Y - 15;
    line_start_x = HEARTRATE_CHART_X;
    line_start_y = HEARTRATE_CHART_Y;
    memset(txt, 0x0, sizeof(txt));

    for (int i = 0; i < 4; i++)
    {
        val = HEART_RATE_MAX - i * 40;
        memset(txt, 0x0, sizeof(txt));
        snprintf(txt, sizeof(txt), "%3d", val);
        value.txt = txt;
        value.ping_pong = 0;
        value.font = &lv_font_montserrat_30;
        value.align = LV_LABEL_ALIGN_LEFT;
        value.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        value.img[0].width = MENU_WIN_XRES - start_x;
        value.img[0].height = lv_font_montserrat_30.line_height;
        value.img[0].stride = MENU_WIN_FB_W * (MENU_WIN_COLOR_DEPTH >> 3);
        value.img[0].pdata = page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3);
        app_lv_label_design(&value);

        buf = (uint16_t *)(page->fb + (line_start_x + line_start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3));
        for (int j = 0; j < HEARTRATE_CHART_W; j++)
        {
            *(buf + j) = 0xFFFF;
        }

        start_y += 40 * HEARTRATE_CHART_H / (HEART_RATE_MAX - HEART_RATE_MIN);
        line_start_y += 40 * HEARTRATE_CHART_H / (HEART_RATE_MAX - HEART_RATE_MIN);
    }

    start_x = HEARTRATE_CHART_X - 24;
    start_y = HEARTRATE_CHART_Y + HEARTRATE_CHART_H + 15;
    for (int i = 0; i < 5; i++)
    {
        val = 0 + i * 6;
        memset(txt, 0x0, sizeof(txt));
        snprintf(txt, sizeof(txt), "%3d", val);
        value.txt = txt;
        value.ping_pong = 0;
        value.font = &lv_font_montserrat_24;
        value.align = LV_LABEL_ALIGN_LEFT;
        value.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
        value.img[0].width = MENU_WIN_XRES - start_x;
        value.img[0].height = lv_font_montserrat_24.line_height;
        value.img[0].stride = MENU_WIN_FB_W * (MENU_WIN_COLOR_DEPTH >> 3);
        value.img[0].pdata = page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3);
        app_lv_label_design(&value);

        start_x += HEARTRATE_CHART_W / 4;
    }
}

static void design_heartbeat_anim(void)
{
    struct app_page_data_t *page = g_func_page;
    int start_x, start_y;
    struct image_st ips, ipd;
    struct image_st ps;

    ps.width = img_heart_info.w;
    ps.height = img_heart_info.h;
    ps.stride = MENU_WIN_XRES * (MENU_WIN_COLOR_DEPTH >> 3);
    ps.pdata = page->fb + (HEARTRATE_ICON_Y) * ps.stride + (HEARTRATE_ICON_X) * (MENU_WIN_COLOR_DEPTH >> 3);

    rk_image_reset(&ps, MSG_WIN_COLOR_DEPTH >> 3);

    start_x = HEARTRATE_ICON_X + scale_val_grp[scale_index] / 2;
    start_y = HEARTRATE_ICON_Y + scale_val_grp[scale_index] / 2;

    ips.width = img_heart_info.w;
    ips.height = img_heart_info.h;
    ips.stride = ips.width * 4;
    ips.pdata = img_heart_info.data;

    ipd.width = img_heart_info.w - scale_val_grp[scale_index];
    ipd.height = img_heart_info.h - scale_val_grp[scale_index];
    ipd.stride = MENU_WIN_FB_W * (MENU_WIN_COLOR_DEPTH >> 3);
    ipd.pdata = page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3);

    rk_scale_process(&ips, &ipd, TYPE_ARGB8888_565);
    scale_index++;
    if (scale_index >= (sizeof(scale_val_grp) / sizeof(int)))
        scale_index = 0;
}

static int last_second = -1;
static page_refrsh_request_param_t g_refr_param;
rt_err_t heartrate_update_design(void *param)
{
    struct app_page_data_t *page = g_func_page;
    struct app_main_data_t *maindata = app_main_data;
    uint8_t *fb;
    struct tm *time;
    int start_x, start_y;
    int first_v, next_v;
    int gap;

    int rate_max = 0, rate_min = 0;
    int rate_change = 0;
    int do_refr = 0;

    app_main_get_time(&time);
    if (maindata->touch_event != RT_TOUCH_EVENT_UP)
        return RT_EOK;

    if (last_second != time->tm_sec && time->tm_sec % 2 == 0)
    {
        last_second = time->tm_sec;
        start_x = HEARTRATE_CHART_X;
        start_y = HEARTRATE_CHART_Y;
        gap = HEARTRATE_CHART_W / 24;
        fb = (uint8_t *)(page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3));

        if (g_rate_cnt == 24)
        {
            struct image_st ps;
            g_rate_cnt = 0;

            g_rate_min = 0xFFFF;
            g_rate_max = -1;

            ps.width = HEARTRATE_CHART_W;
            ps.height = HEARTRATE_CHART_H;
            ps.stride = MENU_WIN_XRES * (MENU_WIN_COLOR_DEPTH >> 3);
            ps.pdata = fb;

            rk_image_reset(&ps, MENU_WIN_COLOR_DEPTH >> 3);
            design_form();
        }

        if (g_rate_cnt)
            first_v = g_rate[g_rate_cnt - 1];
        else
            first_v = HEART_RATE_MIN;

        next_v = rand() % HEART_RATE_MIN + (HEART_RATE_MAX - HEART_RATE_MIN);
        drawLine(fb, g_rate_cnt * gap, (first_v - HEART_RATE_MIN) * HEARTRATE_CHART_H / (HEART_RATE_MAX - HEART_RATE_MIN),
                 (g_rate_cnt + 1) * gap, (next_v - HEART_RATE_MIN) * HEARTRATE_CHART_H / (HEART_RATE_MAX - HEART_RATE_MIN), plot, plot_shadow);
        g_rate[g_rate_cnt] = next_v;
        g_rate_cnt++;

        design_now_value(next_v);

        if (next_v < g_rate_min)
        {
            g_rate_min = next_v;
            rate_change = 1;
        }
        if (next_v > g_rate_max)
        {
            g_rate_max = next_v;
            rate_change = 1;
        }

        if (rate_change)
        {
            if (g_rate_max != -1)
                rate_max = g_rate_max;
            if (g_rate_min != 0xFFFF)
                rate_min = g_rate_min;
            design_value_range(rate_max, rate_min);
        }

        do_refr = 1;
    }

    if (g_anim_delay == 0)
    {
        design_heartbeat_anim();

        do_refr = 1;
    }
    else
    {
        g_anim_delay--;
    }

    if (do_refr)
    {
        g_refr_param.page = page;
        g_refr_param.page_num = 1;
        app_refresh_request(&g_refr_param);
    }

    return RT_EOK;
}
static design_cb_t heartrate_update = {.cb = heartrate_update_design,};

void func_heartrate_update(void)
{
    app_design_request(0, &heartrate_update, NULL);
}

void func_heartrate_exit(void)
{
    struct app_page_data_t *page = g_func_page;

    app_main_timer_cb_unregister();
    rt_free_psram(page->fb);
}

void func_heartrate_enter(void *param)
{
    app_func_show(param);
    app_main_timer_cb_register(func_heartrate_update, 100);
}

void func_heartrate_init(void *param)
{
    struct app_page_data_t *page = g_func_page;
    struct app_func_private *pdata = page->private;
    int start_x, start_y;
    int first_v, next_v = 0;
    int gap;
    int rate_min = 0, rate_max = 0;
    uint8_t *fb;

    /* framebuffer malloc */
    page->fblen = MENU_WIN_XRES * MENU_WIN_YRES * (MENU_WIN_COLOR_DEPTH >> 3);
    page->fb   = (rt_uint8_t *)rt_malloc_psram(page->fblen);
    RT_ASSERT(page->fb != NULL);
    rt_memset((void *)page->fb, 0x0, page->fblen);

    pdata->alpha_win = 0;
    page->w = MENU_WIN_XRES;
    page->h = MENU_WIN_YRES;
    page->vir_w = MENU_WIN_XRES;
    page->ver_offset = 0;
    page->hor_offset = 0;

    design_form();

    start_x = HEARTRATE_CHART_X;
    start_y = HEARTRATE_CHART_Y;
    gap = HEARTRATE_CHART_W / 24;
    fb = (uint8_t *)(page->fb + (start_x + start_y * MENU_WIN_XRES) * (MENU_WIN_COLOR_DEPTH >> 3));
    srand(rt_tick_get());

    first_v = HEART_RATE_MIN;
    next_v = 0;
    for (int i = 0; i < g_rate_cnt; i++)
    {
        next_v = g_rate[i];
        drawLine(fb, i * gap, (first_v - HEART_RATE_MIN) * HEARTRATE_CHART_H / (HEART_RATE_MAX - HEART_RATE_MIN),
                 (i + 1) * gap, (next_v - HEART_RATE_MIN) * HEARTRATE_CHART_H / (HEART_RATE_MAX - HEART_RATE_MIN), plot, plot_shadow);
        first_v = next_v;

        if (next_v < g_rate_min)
            g_rate_min = next_v;
        if (next_v > g_rate_max)
            g_rate_max = next_v;
    }

    design_now_value(next_v);

    if (g_rate_max != -1)
        rate_max = g_rate_max;
    if (g_rate_min != 0xFFFF)
        rate_min = g_rate_min;
    design_value_range(rate_max, rate_min);

    /* anim start after 500ms */
    g_anim_delay = 5;
    scale_index = 0;
    design_heartbeat_anim();

    app_func_set_preview(APP_FUNC_HEARTRATE, page->fb);
}

struct app_func func_heartrate_ops =
{
    .init = func_heartrate_init,
    .enter = func_heartrate_enter,
    .exit = func_heartrate_exit,
};
