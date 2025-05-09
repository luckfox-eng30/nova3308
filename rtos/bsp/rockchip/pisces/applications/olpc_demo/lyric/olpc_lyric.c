/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_LYRIC_ENABLE)

#include "drv_heap.h"
#include "applications/common/image_info.h"
#include "applications/common/olpc_display.h"
#include "applications/common/olpc_ap.h"

#if defined(RT_USING_PISCES_TOUCH)
#include "drv_touch.h"
#include "applications/common/olpc_touch.h"
#endif

/**
 * color palette for 1bpp
 */
static uint32_t lyric_content_lut[2] =
{
    0x001f1f1f, 0x00ffffff
};

static uint32_t lyric_highlight_lut[2] =
{
    0x001f1f1f, 0x0000ffff
};
/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */
#define LYRIC_BACKGOUND_WIN     2
#define LYRIC_CONTENT_WIN       1
#define LYRIC_HIGHLIGHT_WIN     0

#define TITLE_REGION_X          0
#define TITLE_REGION_Y          0
#define TITLE_REGION_W          WIN_LAYERS_W
#define TITLE_REGION_H          WIN_LAYERS_H
#define TITLE_FB_W              720
#define TITLE_FB_H              720

#define ALBUM_REGION_X          0
#define ALBUM_REGION_Y          0
#define ALBUM_REGION_W          WIN_LAYERS_W
#define ALBUM_REGION_H          WIN_LAYERS_H
#define ALBUM_FB_W              720
#define ALBUM_FB_H              720

#define LYRIC_REGION_X          0
#define LYRIC_REGION_Y          0
#define LYRIC_REGION_W          WIN_LAYERS_W
#define LYRIC_REGION_H          WIN_LAYERS_H
#define LYRIC_ROW_W             1024         //must align_32
#define LYRIC_ROW_H             (80 + 40)
#define LYRIC_ROW_NUM           5

/* Event define */
#define EVENT_LYRIC_REFRESH     (0x01UL << 0)
#define EVENT_LYRIC_EXIT        (0x01UL << 1)
#define EVENT_LYRIC_DISP        (0x01UL << 2)

/* Command define */
#define UPDATE_TITLE            (0x01UL << 0)
#define UPDATE_ALBUN            (0x01UL << 1)
#define UPDATE_LYRIC            (0x01UL << 2)
#define UPDATE_HIGHLIGHT        (0x01UL << 3)
#define SCROLL_LYRIC            (0x01UL << 4)

#define LYRIC_VSCROLL_STEP      1
#define LYRIC_HSCROLL_STEP      1
#define PRE_VSROLL_TIME         ((RT_TICK_PER_SECOND / 1000) * 500)
#define LYRIC_VSROLL_TIME       ((RT_TICK_PER_SECOND / 1000) * 20)
#define PRE_HIGHLIGHT_TIME      ((RT_TICK_PER_SECOND / 1000) * 500)
#define UPDATE_HIGHLIGHT_TIME   ((RT_TICK_PER_SECOND / 1000) * 30)

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
//extern image_info_t x_screen00_info;

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_lyric_data
{
    rt_display_data_t disp;

    rt_event_t  disp_event;
    rt_uint32_t cmd;

    rt_int16_t  lrc_id;
    rt_int16_t  lrc_total;

    rt_uint8_t *vs_fb;
    rt_uint32_t vs_fblen;
    rt_timer_t  vs_timer;
    rt_int32_t  vs_yoffset;     /* lrc content fb vertical scroll offset*/

    rt_uint8_t *hs_fb;
    rt_uint32_t hs_fblen;
    rt_timer_t  hs_timer;
    rt_int32_t  hs_xoffset;     /* highlight fb horizontal scroll offset*/
    rt_int32_t  hs_start;       /* highlight fb horizontal scroll max offset*/
    rt_int32_t  hs_xlen;        /* highlight fb horizontal scroll max offset*/
};

static struct olpc_lyric_data *g_olpc_data = RT_NULL;

/*
 **************************************************************************************************
 *
 * lyric test demo
 *
 **************************************************************************************************
 */
extern copy_lv_font_t lyric_fonts;

#define LYRIC_SENTENCE_NUM      10
#define LYRIC_SENTENCE_LEN      16

const static rt_uint16_t lyric_content[LYRIC_SENTENCE_NUM][LYRIC_SENTENCE_LEN] =
{
    /*  花    儿     为     什     么     这      样      红   */
    {33457, 20799, 20026, 20160, 20040, 36825, 26679, 32418, },
    /*  为    什     么     这     样     红 */
    {20026, 20160, 20040, 36825, 26679, 32418, },
    /*  哎           红     得     好     像 */
    {21710,    32, 32418, 24471, 22909, 20687, },
    /*  红    得     好     像     燃     烧      的     火   */
    {32418, 24471, 22909, 20687, 29123, 28903, 30340, 28779, },
    /*  它    象     征     着     纯     洁      的     友     谊     和      爱     情   */
    {23427, 35937, 24449, 30528, 32431, 27905, 30340, 21451, 35850, 21644, 29233, 24773, },
    /*  花    儿     为     什     么     这      样      鲜   */
    {33457, 20799, 20026, 20160, 20040, 36825, 26679, 40092, },
    /*  为    什     么     这     样     鲜 */
    {20026, 20160, 20040, 36825, 26679, 40092, },
    /*  哎           鲜     得     使     人 */
    {21710,    32, 40092, 24471, 20351, 20154, },
    /*  鲜    得     使     人     不     忍      离     去   */
    {40092, 24471, 20351, 20154, 19981, 24525, 31163, 21435, },
    /*  它    是     用     了     青     春      的     血     液      来     浇     灌   */
    {23427, 26159, 29992, 20102, 38738, 26149, 30340, 34880, 28082, 26469, 27975, 28748, },
};

static rt_int16_t olpc_lyric_get_font_id(rt_uint32_t ucode)
{
    rt_uint32_t *ulist = (rt_uint32_t *)lyric_fonts.unicode_list;

    while ((*ulist) && (*ulist != ucode))
    {
        ulist++;
    }

    if (*ulist == ucode)
    {
        return (ulist - lyric_fonts.unicode_list);
    }

    return -1;
}

static rt_uint32_t olpc_lyric_get_font_bitmap(rt_uint32_t ucode)
{
    rt_int16_t id = olpc_lyric_get_font_id(ucode);
    RT_ASSERT(id != -1);
    return (rt_uint32_t)(lyric_fonts.glyph_bitmap + lyric_fonts.glyph_dsc[id].glyph_index);
}

static rt_uint16_t olpc_lyric_get_font_len(rt_uint32_t ucode)
{
    rt_int16_t id = olpc_lyric_get_font_id(ucode);
    RT_ASSERT(id != -1);
    return lyric_fonts.glyph_dsc[id].w_px;
}

static rt_uint16_t olpc_lyric_get_string_len(rt_uint16_t *str)
{
    rt_uint16_t i, len = 0;

    for (i = 0; i < LYRIC_SENTENCE_LEN; i++)
    {
        if (str[i] == 0)
        {
            break;
        }
        len += olpc_lyric_get_font_len((rt_uint32_t)(str[i]));
    }

    return len;
}

static void olpc_lyric_fill_string_bitmap(rt_uint16_t *str, rt_uint8_t *fb, rt_int32_t xVir, rt_int32_t xoffset, rt_int32_t yoffset)
{
    rt_uint16_t i;
    image_info_t img_info;

    rt_memset(&img_info, 0, sizeof(image_info_t));

    img_info.type  = IMG_TYPE_RAW;
    img_info.pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    img_info.x = 0;
    img_info.y = 0;
    img_info.h = lyric_fonts.h_px;

    for (i = 0; i < LYRIC_SENTENCE_LEN; i++)
    {
        if (str[i] == 0)
        {
            break;
        }

        img_info.w    = olpc_lyric_get_font_len((rt_uint32_t)(str[i]));
        img_info.data = (const uint8_t *)olpc_lyric_get_font_bitmap((rt_uint32_t)(str[i]));
        rt_display_img_fill(&img_info, fb, xVir, xoffset, yoffset);

        xoffset += img_info.w;
    }
}

static rt_err_t olpc_lyric_fill_fb_bitmap(struct olpc_lyric_data *olpc_data)
{
    rt_err_t    ret = RT_EOK;
    rt_int32_t  len, xoffset, yoffset;
    rt_int16_t  i, lrc_id, lrc_start = olpc_data->lrc_id;

    xoffset = 0;
    yoffset = 40;
    rt_memset((void *)olpc_data->vs_fb, 0, olpc_data->vs_fblen);
    for (i = 0; i < LYRIC_ROW_NUM + 1; i++)
    {
        lrc_id = lrc_start + i;
        lrc_id -= LYRIC_ROW_NUM / 2;    // align middle of fb

        if (lrc_id >= olpc_data->lrc_total)
        {
            break;
        }

        if (lrc_id >= 0)
        {
            len = olpc_lyric_get_string_len((rt_uint16_t *)lyric_content[lrc_id]);
            RT_ASSERT(len <= LYRIC_ROW_W);
            xoffset = (LYRIC_ROW_W - len) / 2;

            if (i == LYRIC_ROW_NUM / 2)
            {
                olpc_data->hs_xlen  = len;
                olpc_data->hs_start = xoffset;
                olpc_data->hs_xoffset = 0;
                rt_memset(olpc_data->hs_fb, 0, olpc_data->hs_fblen);
            }

            olpc_lyric_fill_string_bitmap((rt_uint16_t *)lyric_content[lrc_id], olpc_data->vs_fb, LYRIC_ROW_W, xoffset, yoffset);
        }

        yoffset += LYRIC_ROW_H;
    }

    return ret;
}

static rt_err_t olpc_lyric_fill_highlight_bitmap(struct olpc_lyric_data *olpc_data)
{
    rt_uint32_t i, j;

    if ((0 < olpc_data->hs_xoffset) && (olpc_data->hs_xoffset <= olpc_data->hs_xlen))
    {
        rt_uint8_t *dst = olpc_data->hs_fb;
        rt_uint8_t *src = olpc_data->vs_fb + LYRIC_ROW_W * (LYRIC_ROW_H * (LYRIC_ROW_NUM / 2)) / 8;

        for (j = 0; j < LYRIC_ROW_H; j++)
        {
            for (i = 0; i < (olpc_data->hs_start + olpc_data->hs_xoffset) / 8; i++)
            {
                dst[j * LYRIC_ROW_W / 8 + i] = src[j * LYRIC_ROW_W / 8 + i];
            }

            rt_uint8_t mod = (olpc_data->hs_start + olpc_data->hs_xoffset) % 8;
            if (mod)
            {
                rt_uint8_t maskval = 0xff >> mod;
                dst[j * LYRIC_ROW_W / 8 + i] &= maskval;
                dst[j * LYRIC_ROW_W / 8 + i] |= (src[j * LYRIC_ROW_W / 8 + i] & (~maskval));
            }
        }
    }

    return RT_EOK;
}

/**
 * Vertical scroll timer callback.
 */
static void olpc_lyric_vscroll_timer(void *parameter)
{
    struct olpc_lyric_data *olpc_data = (struct olpc_lyric_data *)parameter;
    rt_tick_t ticks;

    olpc_data->vs_yoffset += LYRIC_VSCROLL_STEP;
    if (olpc_data->vs_yoffset < LYRIC_ROW_H)
    {
        olpc_data->cmd |= SCROLL_LYRIC;
        rt_event_send(olpc_data->disp_event, EVENT_LYRIC_REFRESH);

        ticks = LYRIC_VSROLL_TIME;
        rt_timer_control(olpc_data->vs_timer, RT_TIMER_CTRL_SET_TIME, &ticks);
        rt_timer_start(olpc_data->vs_timer);
    }
    else
    {
        if (++olpc_data->lrc_id >= olpc_data->lrc_total)
        {
            olpc_data->lrc_id = 0;
        }

        olpc_data->vs_yoffset = 0;
        olpc_data->cmd |= UPDATE_LYRIC;
        rt_event_send(olpc_data->disp_event, EVENT_LYRIC_REFRESH);
    }
}

/**
 * Horizontal scroll timer callback.
 */
static void olpc_lyric_hscroll_timer(void *parameter)
{
    struct olpc_lyric_data *olpc_data = (struct olpc_lyric_data *)parameter;
    rt_tick_t ticks;

    olpc_data->hs_xoffset += LYRIC_HSCROLL_STEP;
    if (olpc_data->hs_xoffset < olpc_data->hs_xlen)
    {
        olpc_data->cmd |= UPDATE_HIGHLIGHT;
        rt_event_send(olpc_data->disp_event, EVENT_LYRIC_REFRESH);

        ticks = UPDATE_HIGHLIGHT_TIME;
        rt_timer_control(olpc_data->hs_timer, RT_TIMER_CTRL_SET_TIME, &ticks);
        rt_timer_start(olpc_data->hs_timer);
    }
    else
    {
        if (olpc_data->lrc_id >= olpc_data->lrc_total - 1)
        {
            olpc_data->lrc_id = 0;              //restart lyric
            olpc_data->vs_yoffset = 0;
            olpc_data->cmd |= UPDATE_LYRIC;
            rt_event_send(olpc_data->disp_event, EVENT_LYRIC_REFRESH);
        }
        else
        {
            ticks = PRE_VSROLL_TIME;
            rt_timer_control(olpc_data->vs_timer, RT_TIMER_CTRL_SET_TIME, &ticks);
            rt_timer_start(olpc_data->vs_timer);    //start vertical scroll
        }
    }
}

/**
 * olpc lyric demo init.
 */
static rt_err_t olpc_lyric_init(struct olpc_lyric_data *olpc_data)
{
    rt_err_t    ret;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    RT_ASSERT(LYRIC_ROW_W % 32 == 0);
    olpc_data->vs_fblen = (LYRIC_ROW_W * (LYRIC_ROW_H * (LYRIC_ROW_NUM + 1))) / 8;
    olpc_data->vs_fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->vs_fblen);
    RT_ASSERT(olpc_data->vs_fb != RT_NULL);

    olpc_data->hs_fblen = (LYRIC_ROW_W * LYRIC_ROW_H) / 8;
    olpc_data->hs_fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->hs_fblen);
    RT_ASSERT(olpc_data->hs_fb != RT_NULL);

    olpc_data->vs_timer = rt_timer_create("lrcvsroll",
                                          olpc_lyric_vscroll_timer,
                                          (void *)olpc_data,
                                          LYRIC_VSROLL_TIME,
                                          RT_TIMER_FLAG_ONE_SHOT);
    RT_ASSERT(olpc_data->vs_timer != RT_NULL);
    //rt_timer_start(olpc_data->vs_timer);

    olpc_data->hs_timer = rt_timer_create("lrchsroll",
                                          olpc_lyric_hscroll_timer,
                                          (void *)olpc_data,
                                          LYRIC_VSROLL_TIME,
                                          RT_TIMER_FLAG_ONE_SHOT);
    RT_ASSERT(olpc_data->hs_timer != RT_NULL);

    olpc_data->lrc_id = 0;
    olpc_data->lrc_total = LYRIC_SENTENCE_NUM;

    return RT_EOK;
}

/**
 * olpc lyric demo deinit.
 */
static void olpc_lyric_deinit(struct olpc_lyric_data *olpc_data)
{
    rt_err_t    ret;

    rt_timer_stop(olpc_data->hs_timer);
    ret = rt_timer_delete(olpc_data->hs_timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->hs_timer = RT_NULL;

    rt_timer_stop(olpc_data->vs_timer);
    ret = rt_timer_delete(olpc_data->vs_timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->vs_timer = RT_NULL;

    rt_free_large((void *)olpc_data->hs_fb);
    olpc_data->hs_fb = RT_NULL;

    rt_free_large((void *)olpc_data->vs_fb);
    olpc_data->vs_fb = RT_NULL;
}

/**
 * olpc lyric vs refresh page
 */
static rt_err_t olpc_lyric_vs_refresh(struct olpc_lyric_data *olpc_data,
                                      struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_device_t  device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId  = LYRIC_CONTENT_WIN;
    wincfg->format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    wincfg->lut    = lyric_content_lut;
    wincfg->lutsize = sizeof(lyric_content_lut) / sizeof(lyric_content_lut[0]);

    RT_ASSERT(olpc_data->vs_yoffset < LYRIC_ROW_H);
    wincfg->fb    = olpc_data->vs_fb + ((LYRIC_ROW_W / 8) * olpc_data->vs_yoffset);
    wincfg->w     = LYRIC_ROW_W;
    wincfg->h     = LYRIC_ROW_H * LYRIC_ROW_NUM;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = LYRIC_REGION_X + (LYRIC_REGION_W - wincfg->w) / 2;
    wincfg->y     = LYRIC_REGION_Y + (LYRIC_REGION_H - wincfg->h) / 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 32) == 0);
    RT_ASSERT((wincfg->y % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->vs_fblen);

    return RT_EOK;
}

/**
 * olpc lyric hs refresh page
 */
static rt_err_t olpc_lyric_hs_refresh(struct olpc_lyric_data *olpc_data,
                                      struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_device_t  device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId   = LYRIC_HIGHLIGHT_WIN;
    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    wincfg->lut     = lyric_highlight_lut;
    wincfg->lutsize = sizeof(lyric_highlight_lut) / sizeof(lyric_highlight_lut[0]);
    wincfg->colorkey = COLOR_KEY_EN | (lyric_highlight_lut[0] & 0x00ffffff);

    wincfg->fb    = olpc_data->hs_fb;
    wincfg->w     = LYRIC_ROW_W;
    wincfg->h     = LYRIC_ROW_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = LYRIC_REGION_X + (LYRIC_REGION_W - wincfg->w) / 2;
    wincfg->y     = LYRIC_REGION_Y + (LYRIC_REGION_H - LYRIC_ROW_H * LYRIC_ROW_NUM) / 2 + LYRIC_ROW_H * 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 32) == 0);
    RT_ASSERT((wincfg->y % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->hs_fblen);

    return RT_EOK;
}

static rt_err_t olpc_lyric_disp_finish(void)
{
    rt_err_t ret;

    ret = rt_event_send(g_olpc_data->disp_event, EVENT_LYRIC_DISP);

    return ret;
}

static rt_err_t olpc_lyric_disp_wait(void)
{
    rt_err_t ret;
    uint32_t event;

    ret = rt_event_recv(g_olpc_data->disp_event, EVENT_LYRIC_DISP,
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                        RT_WAITING_FOREVER, &event);
    RT_ASSERT(ret == RT_EOK);

    return RT_EOK;
}

/**
 * olpc lyric refresh process
 */
static rt_err_t olpc_lyric_task_fun(struct olpc_lyric_data *olpc_data)
{
    rt_err_t    ret;
    struct rt_display_mq_t disp_mq;

    rt_memset(&disp_mq, 0, sizeof(struct rt_display_mq_t));
    disp_mq.win[1].zpos = WIN_MIDDLE_LAYER;
    disp_mq.win[0].zpos = WIN_TOP_LAYER;

    if ((olpc_data->cmd & UPDATE_LYRIC) == UPDATE_LYRIC)
    {
        olpc_data->cmd &= ~UPDATE_LYRIC;

        ret = olpc_lyric_fill_fb_bitmap(olpc_data);
        RT_ASSERT(ret == RT_EOK);
        ret = olpc_lyric_vs_refresh(olpc_data, &disp_mq.win[1]);
        RT_ASSERT(ret == RT_EOK);
        disp_mq.cfgsta |= (0x01 << 1);

        // start h_scroll
        rt_tick_t ticks = PRE_HIGHLIGHT_TIME;
        rt_timer_control(olpc_data->hs_timer, RT_TIMER_CTRL_SET_TIME, &ticks);
        rt_timer_start(olpc_data->hs_timer);
    }
    else if ((olpc_data->cmd & SCROLL_LYRIC) == SCROLL_LYRIC)
    {
        olpc_data->cmd &= ~SCROLL_LYRIC;

        ret = olpc_lyric_vs_refresh(olpc_data, &disp_mq.win[1]);
        RT_ASSERT(ret == RT_EOK);
        disp_mq.cfgsta |= (0x01 << 1);
    }
    else if ((olpc_data->cmd & UPDATE_HIGHLIGHT) == UPDATE_HIGHLIGHT)
    {
        olpc_data->cmd &= ~UPDATE_HIGHLIGHT;

        ret = olpc_lyric_fill_highlight_bitmap(olpc_data);
        RT_ASSERT(ret == RT_EOK);
        ret = olpc_lyric_hs_refresh(olpc_data, &disp_mq.win[0]);
        RT_ASSERT(ret == RT_EOK);
        disp_mq.cfgsta |= (0x01 << 0);

        ret = olpc_lyric_vs_refresh(olpc_data, &disp_mq.win[1]);
        RT_ASSERT(ret == RT_EOK);
        disp_mq.cfgsta |= (0x01 << 1);
    }

    if (disp_mq.cfgsta)
    {
        disp_mq.disp_finish = olpc_lyric_disp_finish;
        ret = rt_mq_send(olpc_data->disp->disp_mq, &disp_mq, sizeof(struct rt_display_mq_t));
        RT_ASSERT(ret == RT_EOK);
        olpc_lyric_disp_wait();
    }

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * olpc lyric touch functions
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)
/**
 * screen touch.
 */
static image_info_t screen_item;
static rt_err_t olpc_lyric_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_lyric_data *olpc_data = (struct olpc_lyric_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_LONG_DOWN:
        rt_event_send(olpc_data->disp_event, EVENT_LYRIC_EXIT);
        return RT_EOK;

    default:
        break;
    }

    return ret;
}

static rt_err_t olpc_lyric_screen_touch_register(void *parameter)
{
    image_info_t *img_info = &screen_item;
    struct olpc_lyric_data *olpc_data = (struct olpc_lyric_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button touch register */
    {
        screen_item.w = WIN_LAYERS_W;
        screen_item.h = WIN_LAYERS_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_lyric_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_lyric_screen_touch_unregister(void *parameter)
{
    image_info_t *img_info = &screen_item;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

#endif

/*
 **************************************************************************************************
 *
 * olpc lyric demo init & thread
 *
 **************************************************************************************************
 */

/**
 * olpc lyric lut set.
 */
static rt_err_t olpc_lyric_display_clear(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut;
    struct olpc_lyric_data *olpc_data = (struct olpc_lyric_data *)parameter;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    lut.winId = LYRIC_BACKGOUND_WIN;
    lut.format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    lut.lut  = RT_NULL;
    lut.size = 0;

    ret = rt_display_lutset(RT_NULL, RT_NULL, &lut);
    RT_ASSERT(ret == RT_EOK);

    // clear screen
    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    rt_display_win_clear(LYRIC_BACKGOUND_WIN, RTGRAPHIC_PIXEL_FORMAT_RGB565, 0, WIN_LAYERS_H, 0);

    return ret;
}

/**
 * olpc lyric dmeo thread.
 */
static void olpc_lyric_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_lyric_data *olpc_data;

    olpc_data = (struct olpc_lyric_data *)rt_malloc(sizeof(struct olpc_lyric_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_lyric_data));
    g_olpc_data = olpc_data;

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_lyric_display_clear(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_lyric_screen_touch_register(olpc_data);
#endif

    olpc_data->disp_event = rt_event_create("display_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->disp_event != RT_NULL);

    ret = olpc_lyric_init(olpc_data);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->cmd = UPDATE_LYRIC;
    rt_event_send(olpc_data->disp_event, EVENT_LYRIC_REFRESH);

    while (1)
    {
        ret = rt_event_recv(olpc_data->disp_event,
                            EVENT_LYRIC_REFRESH | EVENT_LYRIC_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_LYRIC_REFRESH)
        {
            ret = olpc_lyric_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_LYRIC_EXIT)
        {
            break;
        }
    }

    /* Thread deinit */
#if defined(RT_USING_PISCES_TOUCH)
    olpc_lyric_screen_touch_unregister(olpc_data);
    olpc_touch_list_clear();
#endif

    olpc_lyric_deinit(olpc_data);

    rt_event_delete(olpc_data->disp_event);
    olpc_data->disp_event = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;

    rt_event_send(olpc_main_event, EVENT_APP_CLOCK);
}

/**
 * olpc lyric demo application init.
 */
#if defined(OLPC_DLMODULE_ENABLE)
SECTION(".param") rt_uint16_t dlmodule_thread_priority = 5;
SECTION(".param") rt_uint32_t dlmodule_thread_stacksize = 2048;
int main(int argc, char *argv[])
{
    olpc_lyric_thread(RT_NULL);
    return RT_EOK;
}

#else
int olpc_lyric_app_init(void)
{
    rt_thread_t rtt_lyric;

    rtt_lyric = rt_thread_create("olpclyric", olpc_lyric_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(rtt_lyric != RT_NULL);
    rt_thread_startup(rtt_lyric);

    return RT_EOK;
}
//INIT_APP_EXPORT(olpc_lyric_app_init);
#endif

#endif
