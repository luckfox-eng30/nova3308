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

#include <littlevgl2rtt.h>
#include <lvgl/lvgl.h>

#if defined(RT_USING_TOUCH_DRIVERS)
#include "touch.h"
#include "touchpanel.h"
#endif

#include "app_main.h"

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */

#define SHOW_TICK   0

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern lv_font_t lv_font_chnyhei_20;
void lv_font_chnyhei_20_load_psram();
void lv_font_chnyhei_20_free_psram(void);

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct app_page_data_t *g_message_page = RT_NULL;
app_disp_refrsh_param_t app_message_main_refrsh_param;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
static int scale_index = 0;
static int mov_index = MSG_ANIM_STEP;
struct app_lvgl_label_design g_name;
struct app_lvgl_label_design g_no_message;
struct image_st g_pre_tips = {0, 0, 0, NULL};
struct image_st g_pre_area[2] = {{0, 0, 0, NULL}, {0, 0, 0, NULL}};
struct image_st g_pre_txt[2] = {{0, 0, 0, NULL}, {0, 0, 0, NULL}};
static page_refrsh_request_param_t g_refr_param;
static int anim_pause = 0;

static rt_err_t app_lv_new_message_design(void *param);
design_cb_t lv_new_message_design_t = {.cb = app_lv_new_message_design,};
static rt_err_t app_lv_new_message_design(void *param)
{
    struct app_page_data_t *page = g_message_page;
    struct app_msg_private *pdata = (struct app_msg_private *)page->private;
    struct image_st ips, ipd;
    uint32_t start_x, start_y;
    rt_uint8_t buf_id;
    rt_uint8_t *fb;

    if (anim_pause)
        return RT_EOK;

    buf_id = (pdata->buf_id == 1) ? 0 : 1;
    fb = pdata->fb[buf_id];

#if SHOW_TICK
    uint32_t st, et;
    st = HAL_GetTick();
#endif

    ips.width = MSG_LOGO_BIG_W;
    ips.height = MSG_LOGO_BIG_H;
    ips.stride = ips.width * 2;
    ips.pdata = pdata->logo_buf;

    ipd.width = ips.width - (MSG_ANIM_STEP - scale_index) * 6;
    ipd.height = (uint32_t)(ips.height * ((float)ipd.width / ips.width));
    ipd.stride = MSG_WIN_FB_W * (MSG_WIN_COLOR_DEPTH >> 3);
    start_y = MSG_LOGO_Y_START - (int)((float)scale_index * (MSG_LOGO_Y_START - MSG_LOGO_Y_MID) / MSG_ANIM_STEP);
    start_x = (MSG_WIN_XRES - ipd.width) / 2;
    ipd.pdata = fb + start_y * ipd.stride + start_x * (MSG_WIN_COLOR_DEPTH >> 3);
    // if (g_pre_area[buf_id].pdata != NULL)
    //     rk_image_reset(&g_pre_area[buf_id], MSG_WIN_COLOR_DEPTH >> 3);
    if (g_pre_txt[buf_id].pdata != NULL)
        rk_image_reset(&g_pre_txt[buf_id], MSG_WIN_COLOR_DEPTH >> 3);
    memcpy(&g_pre_area[buf_id], &ipd, sizeof(struct image_st));

    rk_scale_process(&ips, &ipd, TYPE_RGB565_565);

    ipd.width = g_name.img[0].width + scale_index * 4;
    ipd.height = (uint32_t)(g_name.img[0].height * ((float)ipd.width / g_name.img[0].width));
    ipd.stride = MSG_WIN_FB_W * (MSG_WIN_COLOR_DEPTH >> 3);
    start_y = MSG_NAME_Y_START - (int)((float)scale_index * (MSG_NAME_Y_START - MSG_NAME_Y_MID) / MSG_ANIM_STEP);
    start_x = (MSG_WIN_XRES - ipd.width) / 2;
    ipd.pdata = fb + start_y * ipd.stride + start_x * (MSG_WIN_COLOR_DEPTH >> 3);
    memcpy(&g_pre_txt[buf_id], &ipd, sizeof(struct image_st));

    rk_scale_process(&g_name.img[0], &ipd, TYPE_RGB565_565);

    pdata->buf_id = buf_id;
    page->fb = pdata->fb[buf_id];

    g_refr_param.page = g_message_page;
    g_refr_param.page_num = 1;
    app_refresh_request(&g_refr_param);

    scale_index++;
    if (scale_index > MSG_ANIM_STEP)
    {
        app_main_register_timeout_cb(app_message_page_show_message, NULL, 1500);
    }
    else
    {
        app_design_request(0, &lv_new_message_design_t, RT_NULL);
    }
#if SHOW_TICK
    et = HAL_GetTick();
    rt_kprintf("design %p [%lu ms]\n", fb, et - st);
#endif

    return RT_EOK;
}

static rt_err_t app_lv_show_message_design(void *param);
design_cb_t lv_show_message_design_t = {.cb = app_lv_show_message_design,};
static rt_err_t app_lv_show_message_design(void *param)
{
    struct app_page_data_t *page = g_message_page;
    struct app_msg_private *pdata = (struct app_msg_private *)page->private;
    struct image_st ips, ipd;
    uint32_t start_x, start_y;
    rt_uint8_t buf_id;
    rt_uint8_t *fb;

    if (anim_pause)
    {
        if (mov_index == MSG_ANIM_STEP)
            mov_index--;
        return RT_EOK;
    }

    buf_id = (pdata->buf_id == 1) ? 0 : 1;
    fb = pdata->fb[buf_id];

#if SHOW_TICK
    uint32_t st, et;
    st = HAL_GetTick();
#endif

    if (mov_index > 0)
    {
        ips.width = MSG_LOGO_BIG_W;
        ips.height = MSG_LOGO_BIG_H;
        ips.stride = ips.width * 2;
        ips.pdata = pdata->logo_buf;

        ipd.width = ips.width - (MSG_ANIM_STEP - mov_index) * 6;
        ipd.height = (uint32_t)(ips.height * ((float)ipd.width / ips.width));
        ipd.stride = MSG_WIN_FB_W * (MSG_WIN_COLOR_DEPTH >> 3);
        start_y = MSG_LOGO_Y_MID + (int)((float)(MSG_ANIM_STEP - mov_index) * ABS(MSG_LOGO_Y_FINAL - MSG_LOGO_Y_MID) / MSG_ANIM_STEP);
        start_x = (MSG_WIN_XRES - ipd.width) / 2;
    }
    else
    {
        ips.width = MSG_LOGO_SMALL_W;
        ips.height = MSG_LOGO_SMALL_H;
        ips.stride = ips.width * 2;
        ips.pdata = pdata->minilogo_buf;

        ipd.width = ips.width;
        ipd.height = ips.height;
        ipd.stride = MSG_WIN_FB_W * (MSG_WIN_COLOR_DEPTH >> 3);
        start_y = MSG_LOGO_Y_FINAL;
        start_x = (MSG_WIN_XRES - ipd.width) / 2;
    }
    ipd.pdata = fb + start_y * ipd.stride + start_x * (MSG_WIN_COLOR_DEPTH >> 3);
    if (g_pre_area[buf_id].pdata != NULL)
        rk_image_reset(&g_pre_area[buf_id], MSG_WIN_COLOR_DEPTH >> 3);
    if (g_pre_txt[buf_id].pdata != NULL)
        rk_image_reset(&g_pre_txt[buf_id], MSG_WIN_COLOR_DEPTH >> 3);
    memcpy(&g_pre_area[buf_id], &ipd, sizeof(struct image_st));

    rk_scale_process(&ips, &ipd, TYPE_RGB565_565);

    ipd.width = g_name.img[0].width + mov_index * 4;//(uint32_t)(g_name.img[0].width * ((float)ipd.width / 2 / ips.width));
    ipd.height = (uint32_t)(g_name.img[0].height * ((float)ipd.width / g_name.img[0].width));
    ipd.stride = MSG_WIN_FB_W * (MSG_WIN_COLOR_DEPTH >> 3);
    start_y = MSG_NAME_Y_MID + (int)((float)(MSG_ANIM_STEP - mov_index) * ABS(MSG_NAME_Y_FINAL - MSG_NAME_Y_MID) / MSG_ANIM_STEP);
    start_x = (MSG_WIN_XRES - ipd.width) / 2;
    ipd.pdata = fb + start_y * ipd.stride + start_x * (MSG_WIN_COLOR_DEPTH >> 3);
    memcpy(&g_pre_txt[buf_id], &ipd, sizeof(struct image_st));

    rk_scale_process(&g_name.img[0], &ipd, TYPE_RGB565_565);

    pdata->offset = (MSG_ANIM_STEP - mov_index) * (MSG_BUF_OFFSET / MSG_ANIM_STEP);

    pdata->buf_id = buf_id;
    page->fb = pdata->fb[buf_id] + pdata->offset * page->vir_w * (MSG_WIN_COLOR_DEPTH >> 3);

    g_refr_param.page = g_message_page;
    g_refr_param.page_num = 1;
    app_refresh_request(&g_refr_param);

    mov_index--;
    if (mov_index < 0)
    {
        scale_index = 0;
        mov_index = MSG_ANIM_STEP;
        if (pdata->msg_cnt)
            pdata->msg_cnt--;
        app_asr_start();
    }
    else
    {
        app_design_request(0, &lv_show_message_design_t, RT_NULL);
    }
#if SHOW_TICK
    et = HAL_GetTick();
    rt_kprintf("design %p [%lu ms]\n", fb, et - st);
#endif

    return RT_EOK;
}

rt_err_t app_message_anim_continue(void)
{
    struct app_page_data_t *page = g_message_page;
    struct app_msg_private *pdata = (struct app_msg_private *)page->private;

    if (!pdata->msg_cnt)
        return -RT_ERROR;

    if (scale_index > MSG_ANIM_STEP && mov_index == MSG_ANIM_STEP)
    {
        anim_pause = 0;
        app_main_register_timeout_cb(app_message_page_show_message, NULL, 1500);
        return RT_EOK;
    }
    if (scale_index <= MSG_ANIM_STEP)
    {
        anim_pause = 0;
        app_design_request(0, &lv_new_message_design_t, RT_NULL);
        return RT_EOK;
    }
    if (mov_index != MSG_ANIM_STEP)
    {
        anim_pause = 0;
        app_design_request(0, &lv_show_message_design_t, RT_NULL);
        return RT_EOK;
    }

    return -RT_ERROR;
}

void app_message_page_exit(void)
{
    struct app_main_data_t *maindata = app_main_data;
    struct app_page_data_t *page = g_message_page;
    struct app_msg_private *pdata = (struct app_msg_private *)page->private;

    if (maindata->ver_page == VER_PAGE_NULL)
        return;
    maindata->ver_page = VER_PAGE_NULL;

    app_asr_start();

    scale_index = 0;
    mov_index = MSG_ANIM_STEP;

    if (g_pre_tips.pdata != NULL)
    {
        rk_image_reset(&g_pre_tips, MSG_WIN_COLOR_DEPTH >> 3);
        memset(&g_pre_tips, 0x0, sizeof(struct image_st));
    }

    if (g_pre_area[0].pdata != NULL)
    {
        rk_image_reset(&g_pre_area[0], MSG_WIN_COLOR_DEPTH >> 3);
        g_pre_area[0].pdata = NULL;
    }
    if (g_pre_area[1].pdata != NULL)
    {
        rk_image_reset(&g_pre_area[1], MSG_WIN_COLOR_DEPTH >> 3);
        g_pre_area[1].pdata = NULL;
    }
    if (g_pre_txt[0].pdata != NULL)
    {
        rk_image_reset(&g_pre_txt[0], MSG_WIN_COLOR_DEPTH >> 3);
        g_pre_txt[0].pdata = NULL;
    }
    if (g_pre_txt[1].pdata != NULL)
    {
        rk_image_reset(&g_pre_txt[1], MSG_WIN_COLOR_DEPTH >> 3);
        g_pre_txt[1].pdata = NULL;
    }
    if (pdata->logo_buf)
    {
        rt_free_large(pdata->logo_buf);
        pdata->logo_buf = NULL;
    }
    if (pdata->minilogo_buf)
    {
        rt_free_large(pdata->minilogo_buf);
        pdata->minilogo_buf = NULL;
    }

    lv_font_chnyhei_20_free_psram();

    pdata->offset = 0;
    page->fb = pdata->fb[pdata->buf_id];
}

static void app_message_txt_design(void)
{
    struct app_page_data_t *page = g_message_page;
    struct app_msg_private *pdata = (struct app_msg_private *)page->private;
    struct tm *time;
    struct app_lvgl_label_design msg_content;
    uint32_t start_x, start_y;
    char *str;

    app_main_get_time(&time);
    g_name.txt = "隔壁老王";
    g_name.ping_pong = 0;
    g_name.font = &lv_font_chnyhei_20;
    g_name.align = LV_LABEL_ALIGN_CENTER;
    g_name.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    g_name.img[0].width = FUNC_WIN_XRES;
    g_name.img[0].height = lv_font_chnyhei_20.line_height;
    g_name.img[0].stride = FUNC_WIN_XRES * (MSG_WIN_COLOR_DEPTH >> 3);
    if (g_name.img[0].pdata != NULL)
    {
        rt_free_psram(g_name.img[0].pdata);
    }
    g_name.img[0].pdata = rt_malloc_psram(g_name.img[0].stride * g_name.img[0].height);
    RT_ASSERT(g_name.img[0].pdata != NULL);
    app_lv_label_design(&g_name);

    str = rt_malloc(1024);
    RT_ASSERT(str != NULL);
    rt_snprintf(str, 1024,
                "\n%02d:%02d\n"
                "Hello，老王！"
                "我家水管坏了，"
                "您能过来帮我修一下吗？\n "
                "Thanks！！！",
                time->tm_hour, time->tm_min, '\0');

    msg_content.txt = str;
    msg_content.ping_pong = 1;
    msg_content.font = &lv_font_chnyhei_20;
    msg_content.align = LV_LABEL_ALIGN_CENTER;
    msg_content.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    msg_content.img[1].width = msg_content.img[0].width = MSG_WIN_FB_W - 20;
    msg_content.img[1].height = msg_content.img[0].height = MSG_WIN_YRES;
    msg_content.img[1].stride = msg_content.img[0].stride = MSG_WIN_FB_W * (MSG_WIN_COLOR_DEPTH >> 3);
    start_x = ((MSG_WIN_XRES - LV_HOR_RES) / 2 + 10);
    start_y = MSG_WIN_YRES;
    msg_content.img[0].pdata = pdata->fb[0] + start_y * msg_content.img[0].stride + start_x * (MSG_WIN_COLOR_DEPTH >> 3);
    msg_content.img[1].pdata = pdata->fb[1] + start_y * msg_content.img[1].stride + start_x * (MSG_WIN_COLOR_DEPTH >> 3);
    app_lv_label_design(&msg_content);

    rt_free(str);
}

rt_err_t app_message_page_new_message(void *param)
{
    struct app_page_data_t *page = g_message_page;
    struct app_msg_private *pdata = (struct app_msg_private *)page->private;
    pdata->buf_id = 0;
    scale_index = 0;
    app_asr_stop();

    // pdata->msg_cnt++;
    pdata->msg_cnt = 1;
    anim_pause = 0;

    pdata->offset = 0;
    page->fb = pdata->fb[pdata->buf_id];
    if (g_pre_tips.pdata != NULL)
    {
        rk_image_reset(&g_pre_tips, MSG_WIN_COLOR_DEPTH >> 3);
        memset(&g_pre_tips, 0x0, sizeof(struct image_st));
    }

    pdata->logo_buf = (rt_uint8_t *)rt_malloc_large(MSG_LOGO_BIG_W * MSG_LOGO_BIG_H * 2);
    RT_ASSERT(pdata->logo_buf != NULL);
    pdata->minilogo_buf = (rt_uint8_t *)rt_malloc_large(MSG_LOGO_SMALL_W * MSG_LOGO_SMALL_H * 2);
    RT_ASSERT(pdata->minilogo_buf != NULL);

    img_load_info_t img_load_info;
    img_load_info.w = MSG_LOGO_BIG_W;
    img_load_info.h = MSG_LOGO_BIG_H;
    img_load_info.name = ICONS_PATH"/WhatsApp_160x160.dta";
    app_load_img(&img_load_info, pdata->logo_buf, MSG_LOGO_BIG_W, MSG_LOGO_BIG_H, 0, 2);

    img_load_info.w = MSG_LOGO_SMALL_W;
    img_load_info.h = MSG_LOGO_SMALL_H;
    img_load_info.name = ICONS_PATH"/WhatsApp_64x64.dta";
    app_load_img(&img_load_info, pdata->minilogo_buf, MSG_LOGO_SMALL_W, MSG_LOGO_SMALL_H, 0, 2);

    lv_font_chnyhei_20_load_psram();

    app_message_txt_design();

    app_design_request(0, &lv_new_message_design_t, RT_NULL);

    return RT_EOK;
}

rt_err_t app_message_page_show_message(void *param)
{
    mov_index = MSG_ANIM_STEP;
    app_design_request(0, &lv_show_message_design_t, RT_NULL);

    return RT_EOK;
}

static rt_err_t app_message_page_show_tips(void *param)
{
    struct app_lvgl_label_design *design = (struct app_lvgl_label_design *)param;
    struct app_page_data_t *page = g_message_page;
    struct app_msg_private *pdata = (struct app_msg_private *)page->private;
    uint32_t start_x, start_y;

    pdata->offset = 0;
    page->fb = pdata->fb[pdata->buf_id];
    if (g_pre_area[0].pdata != NULL)
    {
        rk_image_reset(&g_pre_area[0], MSG_WIN_COLOR_DEPTH >> 3);
        g_pre_area[0].pdata = NULL;
    }
    if (g_pre_area[1].pdata != NULL)
    {
        rk_image_reset(&g_pre_area[1], MSG_WIN_COLOR_DEPTH >> 3);
        g_pre_area[1].pdata = NULL;
    }
    if (g_pre_txt[0].pdata != NULL)
    {
        rk_image_reset(&g_pre_txt[0], MSG_WIN_COLOR_DEPTH >> 3);
        g_pre_txt[0].pdata = NULL;
    }
    if (g_pre_txt[1].pdata != NULL)
    {
        rk_image_reset(&g_pre_txt[1], MSG_WIN_COLOR_DEPTH >> 3);
        g_pre_txt[1].pdata = NULL;
    }

    g_pre_tips.width = design->img[0].width;
    g_pre_tips.height = design->img[0].height;
    g_pre_tips.stride = MSG_WIN_FB_W * (MSG_WIN_COLOR_DEPTH >> 3);
    start_x = (MSG_WIN_XRES - g_pre_tips.width) / 2;
    start_y = (MSG_WIN_YRES - g_pre_tips.height) / 2;
    g_pre_tips.pdata = pdata->fb[pdata->buf_id] + start_y * g_pre_tips.stride + start_x * (MSG_WIN_COLOR_DEPTH >> 3);
    rk_image_copy(&design->img[0], &g_pre_tips, MSG_WIN_COLOR_DEPTH >> 3);

    g_refr_param.page = g_message_page;
    g_refr_param.page_num = 1;
    app_refresh_request(&g_refr_param);

    return RT_EOK;
}
design_cb_t message_page_show_tips_t = {.cb = app_message_page_show_tips,};

rt_err_t app_message_page_check_message(void *param)
{
    struct app_page_data_t *page = g_message_page;
    struct app_msg_private *pdata = (struct app_msg_private *)page->private;

    if (pdata->msg_cnt == 0)
    {
        if (g_no_message.img[0].pdata == NULL)
        {
            g_no_message.txt = "No Message";
            g_no_message.ping_pong = 0;
            g_no_message.font = &lv_font_montserrat_44;
            g_no_message.align = LV_LABEL_ALIGN_CENTER;
            g_no_message.fmt = RTGRAPHIC_PIXEL_FORMAT_RGB565;
            g_no_message.img[0].width = MSG_WIN_XRES;
            g_no_message.img[0].height = lv_font_montserrat_44.line_height;
            g_no_message.img[0].stride = MSG_WIN_XRES * (MSG_WIN_COLOR_DEPTH >> 3);
            g_no_message.img[0].pdata = rt_malloc_psram(g_no_message.img[0].stride * g_no_message.img[0].height);
            RT_ASSERT(g_no_message.img[0].pdata != RT_NULL);
            memset(g_no_message.img[0].pdata, 0x0, g_no_message.img[0].stride * g_no_message.img[0].height);
            app_design_request(0, &lv_lvgl_label_design_t, &g_no_message);
        }
        app_design_request(0, &message_page_show_tips_t, &g_no_message);
    }

    return RT_EOK;
}

static rt_err_t app_message_move_updn_design(void *param);
design_cb_t message_move_updn_design = { .cb = app_message_move_updn_design, };
static rt_err_t app_message_move_updn_design(void *param)
{
    struct app_page_data_t *page = g_message_page;
    mov_design_param *tar = (mov_design_param *)param;

    page->ver_offset += TOUCH_MOVE_STEP * tar->dir;
    if ((tar->dir * page->ver_offset) >= (tar->dir * tar->offset))
    {
        if (tar->offset != 0)
        {
            app_main_touch_unregister();
            app_main_touch_register(&main_page_touch_cb);
            if (main_page_timer_cb[app_main_page->hor_page].cb)
            {
                app_main_timer_cb_register(main_page_timer_cb[app_main_page->hor_page].cb,
                                           main_page_timer_cb[app_main_page->hor_page].cycle_ms);
            }
            app_message_page_exit();
            app_update_page(app_main_page);
            g_refr_param.page = app_main_page;
            g_refr_param.page_num = 1;
        }
        else
        {
            app_message_anim_continue();
            g_refr_param.page = page;
            g_refr_param.page_num = 1;
        }
        page->ver_offset = tar->offset;

        app_refresh_request(&g_refr_param);
    }
    else // continue move
    {
        page->next = app_main_page;
        g_refr_param.page = page;
        g_refr_param.page_num = 2;
        g_refr_param.auto_resize = 1;
        app_refresh_request(&g_refr_param);

        app_design_request(0, &message_move_updn_design, param);
    }

    return RT_EOK;
}

static mov_design_param touch_moveup_design_param;
rt_err_t app_message_touch_move_up(void *param)
{
    struct app_main_data_t *pdata = (struct app_main_data_t *)param;
    struct rt_touch_data *cur_p   = &pdata->cur_point[0];
    struct rt_touch_data *down_p   = &pdata->down_point[0];

    if (pdata->dir_mode == TOUCH_DIR_MODE_UPDN)
    {
        app_slide_refresh_undo();
        if ((down_p->y_coordinate >= cur_p->y_coordinate))
        {
            app_message_anim_continue();
            return RT_EOK;
        }

        if ((cur_p->y_coordinate - down_p->y_coordinate) >= MSG_WIN_YRES / 6)
        {
            touch_moveup_design_param.dir = -1;
            touch_moveup_design_param.offset = MSG_WIN_YRES * touch_moveup_design_param.dir;
        }
        else
        {
            touch_moveup_design_param.dir = 1;
            touch_moveup_design_param.offset = 0;
        }
        app_design_request(0, &message_move_updn_design, &touch_moveup_design_param);
    }

    return RT_EOK;
}

static void slide_updn_cb(int mov_fix)
{
    struct app_page_data_t *page = g_message_page;
    page->ver_offset -= mov_fix;
}

rt_err_t app_message_touch_move_updn(void *param)
{
    struct app_main_data_t *pdata = (struct app_main_data_t *)param;
    struct app_page_data_t *page = g_message_page;

    if (pdata->yoffset < 0)
        return RT_EOK;

    page->ver_offset = -pdata->yoffset;

    page->next = app_main_page;
    g_refr_param.page = page;
    g_refr_param.page_num = 2;
    g_refr_param.auto_resize = 1;
    g_refr_param.cb = slide_updn_cb;
    app_slide_refresh(&g_refr_param);

    return RT_EOK;
}

static rt_err_t app_message_touch_up(void *param)
{
    if (app_message_anim_continue() == -RT_ERROR)
    {
        app_message_page_check_message(param);
    }

    return RT_EOK;
}

static rt_err_t app_message_touch_down(void *param)
{
    app_main_unregister_timeout_cb_if_is(app_message_page_show_message);
    anim_pause = 1;

    app_main_get_time(&app_main_data->tmr_data);
    app_main_page_clock_design(NULL);

    return RT_EOK;
};

extern rt_err_t app_main_page_touch_move_updn(void *param);
struct app_touch_cb_t app_message_main_touch_cb =
{
    .tp_touch_down  = app_message_touch_down,
    .tp_move_updn   = app_message_touch_move_updn,
    .tp_move_up     = app_message_touch_move_up,
    .tp_touch_up    = app_message_touch_up,
};

static rt_err_t app_message_main_init_design(void *param)
{
    struct app_page_data_t *page;
    struct app_msg_private *pdata;

    g_message_page = page = (struct app_page_data_t *)rt_malloc(sizeof(struct app_page_data_t));
    RT_ASSERT(page != RT_NULL);
    pdata = (struct app_msg_private *)rt_malloc(sizeof(struct app_msg_private));
    RT_ASSERT(pdata != RT_NULL);
    rt_memset((void *)page, 0, sizeof(struct app_page_data_t));
    rt_memset((void *)pdata, 0, sizeof(struct app_page_data_t));

    page->id = ID_NONE;
    page->w = MSG_WIN_XRES;
    page->h = MSG_WIN_YRES;
    page->vir_w = MSG_WIN_FB_W;
    page->exit_side = EXIT_SIDE_TOP;
    page->win_id = APP_CLOCK_WIN_1;
    page->win_layer = WIN_MIDDLE_LAYER;
    page->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    page->fblen = MSG_WIN_FB_W * MSG_WIN_FB_H * MSG_WIN_COLOR_DEPTH / 8;
    page->private = pdata;
    page->touch_cb = &app_message_main_touch_cb;

    page->fb = pdata->fb[0] = (rt_uint8_t *)rt_malloc_psram(page->fblen);
    pdata->fb[1] = (rt_uint8_t *)rt_malloc_psram(page->fblen);
    RT_ASSERT(pdata->fb[0] != RT_NULL);
    RT_ASSERT(pdata->fb[1] != RT_NULL);
    memset(pdata->fb[0], 0, page->fblen);
    memset(pdata->fb[1], 0, page->fblen);
    pdata->buf_id = 0;
    pdata->msg_cnt = 0;
    memset(&g_name, 0x0, sizeof(struct app_lvgl_label_design));
    memset(&g_no_message, 0x0, sizeof(struct app_lvgl_label_design));

    app_main_page->bottom = page;
    page->top = app_main_page;

    return RT_EOK;
}
static design_cb_t  app_message_main_init_design_t = {.cb = app_message_main_init_design,};

/**
 * App clock fast init.
 */
void app_message_main_init(void)
{
    app_design_request(0, &app_message_main_init_design_t, RT_NULL);
}
