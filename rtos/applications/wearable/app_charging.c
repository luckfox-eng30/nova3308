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

#if defined(RT_USING_TOUCH_DRIVERS)
#include "touch.h"
#include "touchpanel.h"
#endif

#include "app_main.h"
#include "lib_imageprocess.h"

#define SHOW_TICK           0

uint32_t m_bpp_lut[256] = {0};
static int g_first_dta;

struct app_page_data_t *g_charging_page[2];
app_disp_refrsh_param_t app_charging_refrsh_param;
app_disp_refrsh_param_t app_charging_refrsh_lut_param;
static void app_charing_anim_unload(void);

#if SHOW_TICK
uint32_t anim_st, anim_et;
uint32_t algo_t = 0;
#endif

static int anim_index = 1;
static page_refrsh_request_param_t clock_anim_refr_param;
static rt_err_t app_charging_anim_design(void *param);
design_cb_t charging_anim_design_t = {.cb = app_charging_anim_design,};
static rt_err_t app_charging_anim_design(void *param)
{
    struct app_page_data_t *page = g_charging_page[0];
    struct app_charging_private *pdata = (struct app_charging_private *)page->private;
    rt_uint8_t buf_id;
    rt_uint8_t *fb;

    rt_uint8_t *src_buf;
    rt_uint32_t src_size;
    rt_uint32_t dec_size;
    int lut_index;

    if (pdata->enable == 0)
    {
        rt_free_psram(pdata->fb[0]);
        rt_free_psram(pdata->fb[1]);
        app_charing_anim_unload();

        clock_anim_refr_param.page = app_main_page;
        clock_anim_refr_param.page_num = 1;
        app_refresh_request(&clock_anim_refr_param);

        app_main_touch_unregister();
        app_main_touch_register(&main_page_touch_cb);
        if (main_page_timer_cb[app_main_page->hor_page].cb)
        {
            app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                       main_page_timer_cb[app_main_page->hor_page].cycle_ms);
        }

        app_asr_start();

        return RT_EOK;
    }

    buf_id = (pdata->buf_id == 1) ? 0 : 1;
    fb = pdata->fb[buf_id];

#if SHOW_TICK
    uint32_t st, et;
    st = HAL_GetTick();
#endif

    lut_index = anim_index;
    for (int i = 0; i < 256; i++)
    {
        m_bpp_lut[i] = pdata->anim_lut[lut_index][i];
    }

    uint32_t dec_s, dec_e;
    dec_s = HAL_GetTick();
    dec_size = CHARGING_WIN_FB_W * CHARGING_WIN_FB_H;
    if (!g_first_dta)
    {
        src_size = pdata->anim_buflen[anim_index];
        src_buf = (rt_uint8_t *)pdata->anim_buf[anim_index];
        rk_lzw_decompress_dsp(src_buf, src_size, fb, dec_size);
    }
    else
    {
        memset(fb, 0x0, dec_size);
        g_first_dta = 0;
    }
    dec_e = HAL_GetTick();

#if SHOW_TICK
    et = HAL_GetTick();
    algo_t += et - st;

    if (anim_index == pdata->reload_idx)
        anim_st = HAL_GetTick();
#endif
    anim_index++;
    if (anim_index >= pdata->steps)
    {
        anim_index = pdata->reload_idx;
#if SHOW_TICK
        anim_et = HAL_GetTick();
        rt_kprintf("Design %p [%lu ms] FPS %d:algo %lu ms\n", fb, anim_et - anim_st,
                   1000 * (pdata->steps - pdata->reload_idx) / (anim_et - anim_st), algo_t);
        algo_t = 0;
#endif
    }
    if ((dec_e - dec_s) < 30)
    {
        rt_thread_mdelay(30 - (dec_e - dec_s));
    }

    pdata->buf_id = buf_id;

    g_charging_page[buf_id]->h = CHARGING_WIN_YRES;
    g_charging_page[buf_id]->win_layer = WIN_TOP_LAYER;
    g_charging_page[buf_id]->new_lut = 1;
    g_charging_page[buf_id]->hide_win = 0;
    g_charging_page[1 - buf_id]->h = 2;
    g_charging_page[1 - buf_id]->win_layer = WIN_BOTTOM_LAYER;
    g_charging_page[1 - buf_id]->new_lut = 0;
    g_charging_page[1 - buf_id]->hide_win = 1;

    g_charging_page[0]->fb = pdata->fb[pdata->buf_id];
    g_charging_page[1]->fb = pdata->fb[pdata->buf_id];

    g_charging_page[0]->next = g_charging_page[1];
    clock_anim_refr_param.page = g_charging_page[0];
    clock_anim_refr_param.page_num = 2;
    app_refresh_request(&clock_anim_refr_param);

    app_design_request(0, &charging_anim_design_t, RT_NULL);

    return RT_EOK;
}

rt_err_t app_charging_touch_up(void *param)
{
    struct app_page_data_t *page = g_charging_page[0];
    struct app_charging_private *pdata = (struct app_charging_private *)page->private;

    pdata->enable = 0;

    return RT_EOK;
}

struct app_touch_cb_t app_charging_touch_cb =
{
    .tp_touch_up = app_charging_touch_up,
};

static void app_charing_anim_unload(void)
{
    struct app_page_data_t *page = g_charging_page[0];
    struct app_charging_private *pdata = (struct app_charging_private *)page->private;

    rt_free_psram(pdata->anim_dta);
    pdata->anim_loaded = 0;
}

static void app_charing_anim_load_source(char *dta_path, int steps, int reload_idx)
{
    struct app_page_data_t *page = g_charging_page[0];
    struct app_charging_private *pdata = (struct app_charging_private *)page->private;
    struct stat dta_buf;
    uint32_t ofs;
    FILE *fd;

    if (pdata->anim_loaded)
        return;

    pdata->steps = steps;
    pdata->reload_idx = reload_idx;

    // uint32_t st, et;
    // st = HAL_GetTick();

    if (stat(dta_path, &dta_buf) == 0)
    {
        pdata->anim_dta = rt_malloc_psram(dta_buf.st_size);
        RT_ASSERT(pdata->anim_dta != NULL);
        fd = fopen(dta_path, "r");
        fread(pdata->anim_dta, 1, dta_buf.st_size, fd);
        fclose(fd);
    }

    ofs = 0;
    for (int i = 0; i < steps; i++)
    {
        pdata->anim_lut[i] = (uint32_t *)(pdata->anim_dta + ofs);
        ofs += sizeof(m_bpp_lut);
        pdata->anim_buflen[i] = *(uint32_t *)(pdata->anim_dta + ofs);
        ofs += sizeof(uint32_t);
        pdata->anim_buf[i] = pdata->anim_dta + ofs;
        ofs += pdata->anim_buflen[i];
    }

    // et = HAL_GetTick();
    // rt_kprintf("%s %d cast %lu ms\n", dta_path, dta_buf.st_size, et - st);

    pdata->anim_loaded = 1;
}

static rt_err_t app_start_charging_anim(void *param)
{
    struct app_page_data_t *page = g_charging_page[0];
    struct app_charging_private *pdata = (struct app_charging_private *)page->private;

    app_asr_stop();
    app_charing_anim_load_source(ANIM_SRCPATH"/charging.dta", CHARGING_ANIM_STEP, CHARGING_ANIM_LOOP_START);

    pdata->fb[0] = (rt_uint8_t *)rt_malloc_psram(pdata->fblen);
    pdata->fb[1] = (rt_uint8_t *)rt_malloc_psram(pdata->fblen);
    RT_ASSERT(pdata->fb[0] != RT_NULL);
    RT_ASSERT(pdata->fb[1] != RT_NULL);
    pdata->buf_id = 0;
    g_charging_page[0]->fb = pdata->fb[pdata->buf_id];
    g_charging_page[1]->fb = pdata->fb[pdata->buf_id];
    g_charging_page[0]->fblen = pdata->fblen;
    g_charging_page[1]->fblen = pdata->fblen;

    g_first_dta = 1;
    app_message_page_exit();
    app_main_timer_cb_unregister();
    app_main_touch_unregister();
    app_main_touch_register(&app_charging_touch_cb);

    app_design_request(0, &charging_anim_design_t, RT_NULL);

    return RT_EOK;
}
design_cb_t start_charging_anim_t = {.cb = app_start_charging_anim,};

rt_err_t app_charging_enable(void *param)
{
    struct app_page_data_t *page = g_charging_page[0];
    struct app_charging_private *pdata = (struct app_charging_private *)page->private;

    if (pdata->enable == 0)
    {
        pdata->enable = 1;
        app_design_request(0, &start_charging_anim_t, RT_NULL);
    }

    return RT_EOK;
}

MSH_CMD_EXPORT(app_charging_enable, charging);

static rt_err_t app_charging_init_design(void *param)
{
    struct app_page_data_t *page = g_charging_page[0];
    struct app_charging_private *pdata = (struct app_charging_private *)page->private;

    g_charging_page[0] = (struct app_page_data_t *)rt_malloc(sizeof(struct app_page_data_t));
    RT_ASSERT(g_charging_page[0] != RT_NULL);
    g_charging_page[1] = (struct app_page_data_t *)rt_malloc(sizeof(struct app_page_data_t));
    RT_ASSERT(g_charging_page[1] != RT_NULL);
    pdata = (struct app_charging_private *)rt_malloc(sizeof(struct app_charging_private));
    RT_ASSERT(pdata != RT_NULL);

    rt_memset((void *)g_charging_page[0], 0, sizeof(struct app_page_data_t));
    rt_memset((void *)pdata, 0, sizeof(struct app_charging_private));

    g_charging_page[0]->id = ID_NONE;
    g_charging_page[0]->w = CHARGING_WIN_XRES;
    g_charging_page[0]->vir_w = CHARGING_WIN_FB_W;
    g_charging_page[0]->hor_step = 0;
    g_charging_page[0]->format = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    g_charging_page[0]->private = pdata;
    g_charging_page[0]->lut = m_bpp_lut;
    g_charging_page[0]->lutsize = sizeof(m_bpp_lut) / sizeof(m_bpp_lut[0]);
    g_charging_page[0]->touch_cb = &app_charging_touch_cb;
    rt_memcpy(g_charging_page[1], g_charging_page[0], sizeof(struct app_page_data_t));

    g_charging_page[0]->h = CHARGING_WIN_YRES;
    g_charging_page[0]->win_id = APP_CLOCK_WIN_1;
    g_charging_page[0]->win_layer = WIN_MIDDLE_LAYER;

    g_charging_page[1]->h = 2;
    g_charging_page[1]->win_id = APP_CLOCK_WIN_0;
    g_charging_page[1]->win_layer = WIN_BOTTOM_LAYER;

    pdata->fblen = CHARGING_WIN_FB_W * CHARGING_WIN_FB_H * (CHARGING_WIN_COLOR_DEPTH >> 3);
    pdata->enable = 0;
    pdata->anim_loaded = 0;

    return RT_EOK;
}
static design_cb_t  app_charging_init_design_t = {.cb = app_charging_init_design,};

/**
 * App clock fast init.
 */
void app_charging_init(void)
{
    app_design_request(0, &app_charging_init_design_t, RT_NULL);
}
