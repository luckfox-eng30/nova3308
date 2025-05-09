/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_NOTE_ENABLE)

#include "drv_heap.h"
#include "applications/common/image_info.h"
#include "applications/common/olpc_display.h"

#if defined(RT_USING_PISCES_TOUCH)
#include "drv_touch.h"
#include "applications/common/olpc_touch.h"
#endif

/**
 * color palette for 1bpp
 */
static uint32_t bpp1_lut[2] =
{
    0x00AAAAAA, 0x000000
    //0x000000, 0x00FFFFFF
};

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */
#define NOTE_TEST

#define NOTE_TEXT_GRAY1_WIN    0
#define NOTE_CTRL_RGB332_WIN   1

#define NOTE_CTRL_REGION_X     ((WIN_LAYERS_W - NOTE_CTRL_FB_W) / 2)
#define NOTE_CTRL_REGION_Y     60
#define NOTE_CTRL_REGION_W     ((WIN_LAYERS_W / 32) * 32)
#define NOTE_CTRL_REGION_H     90
#define NOTE_CTRL_FB_W         NOTE_CTRL_REGION_W
#define NOTE_CTRL_FB_H         NOTE_CTRL_REGION_H

#define NOTE_CTRL1_REGION_X    ((WIN_LAYERS_W - NOTE_CTRL_FB_W) / 2)
#define NOTE_CTRL1_REGION_Y    180
#define NOTE_CTRL1_REGION_W    ((WIN_LAYERS_W / 32) * 32)
#define NOTE_CTRL1_REGION_H    80
#define NOTE_CTRL1_FB_W        NOTE_CTRL1_REGION_W
#define NOTE_CTRL1_FB_H        NOTE_CTRL1_REGION_H

#define NOTE_DRAW_REGION_X     ((WIN_LAYERS_W - NOTE_DRAW_FB_W) / 2)
#define NOTE_DRAW_REGION_Y     180
#define NOTE_DRAW_REGION_W     ((WIN_LAYERS_W / 32) * 32)
#define NOTE_DRAW_REGION_H     2100
#define NOTE_DRAW_FB_W         NOTE_DRAW_REGION_W
#define NOTE_DRAW_FB_H         NOTE_DRAW_REGION_H

/* Event define */
#define EVENT_NOTE_REFRESH     (0x01UL << 0)
#define EVENT_NOTE_EXIT        (0x01UL << 1)
#define EVENT_SRCSAVER_ENTER   (0x01UL << 2)
#define EVENT_SRCSAVER_EXIT    (0x01UL << 3)

/* Command define */
#define UPDATE_DRAW            (0x01UL << 0)
#define UPDATE_REVIEW_IMG      (0x01UL << 1)
#define UPDATE_DRAW_IMG        (0x01UL << 2)
#define UPDATE_DRAW_CLEAR      (0x01UL << 3)
#define UPDATE_CTRL            (0x01UL << 4)
#define UPDATE_SUBCTRL         (0x01UL << 5)

#define NOTE_SRCSAVER_TIME     (RT_TICK_PER_SECOND * 15)
#define NOTE_SUBCTRL_TIME      (RT_TICK_PER_SECOND * 5)

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern image_info_t note_btnprev0_info;
extern image_info_t note_btnprev1_info;

extern image_info_t note_btnline0_info;
extern image_info_t note_btnline1_info;

extern image_info_t note_btnrub0_info;
extern image_info_t note_btnrub1_info;

extern image_info_t note_btnsave0_info;
extern image_info_t note_btnsave1_info;

extern image_info_t note_btndel0_info;
extern image_info_t note_btndel1_info;

extern image_info_t note_btnexit0_info;
extern image_info_t note_btnexit1_info;

extern image_info_t note_btnnext0_info;
extern image_info_t note_btnnext1_info;

extern image_info_t note_subbtnline040_info;
extern image_info_t note_subbtnline041_info;
extern image_info_t note_subbtnline080_info;
extern image_info_t note_subbtnline081_info;
extern image_info_t note_subbtnline120_info;
extern image_info_t note_subbtnline121_info;
extern image_info_t note_subbtnline160_info;
extern image_info_t note_subbtnline161_info;
extern image_info_t note_subbtnline200_info;
extern image_info_t note_subbtnline201_info;
extern image_info_t note_subbtnline240_info;
extern image_info_t note_subbtnline241_info;
extern image_info_t note_subbtnline280_info;
extern image_info_t note_subbtnline281_info;
extern image_info_t note_subbtnline320_info;
extern image_info_t note_subbtnline321_info;

extern image_info_t note_subbtnrub040_info;
extern image_info_t note_subbtnrub041_info;
extern image_info_t note_subbtnrub080_info;
extern image_info_t note_subbtnrub081_info;
extern image_info_t note_subbtnrub120_info;
extern image_info_t note_subbtnrub121_info;
extern image_info_t note_subbtnrub160_info;
extern image_info_t note_subbtnrub161_info;
extern image_info_t note_subbtnrub200_info;
extern image_info_t note_subbtnrub201_info;
extern image_info_t note_subbtnrub240_info;
extern image_info_t note_subbtnrub241_info;
extern image_info_t note_subbtnrub280_info;
extern image_info_t note_subbtnrub281_info;
extern image_info_t note_subbtnrub320_info;
extern image_info_t note_subbtnrub321_info;

extern image_info_t note_dot040_info;
extern image_info_t note_dot041_info;
extern image_info_t note_dot080_info;
extern image_info_t note_dot081_info;
extern image_info_t note_dot120_info;
extern image_info_t note_dot121_info;
extern image_info_t note_dot160_info;
extern image_info_t note_dot161_info;
extern image_info_t note_dot200_info;
extern image_info_t note_dot201_info;
extern image_info_t note_dot240_info;
extern image_info_t note_dot241_info;
extern image_info_t note_dot280_info;
extern image_info_t note_dot281_info;
extern image_info_t note_dot320_info;
extern image_info_t note_dot321_info;

static image_info_t screen_item;
static image_info_t draw_item;

#ifdef NOTE_TEST
extern image_info_t note_prevpage_info;
static image_info_t save_item;
#endif

#if defined(RT_USING_PISCES_TOUCH)
static rt_err_t olpc_note_subbtnline_touch_register(void *parameter);
static rt_err_t olpc_note_subbtnline_touch_unregister(void *parameter);

static rt_err_t olpc_note_subbtnrub_touch_register(void *parameter);
static rt_err_t olpc_note_subbtnrub_touch_unregister(void *parameter);
#endif
/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_note_data
{
    rt_display_data_t disp;
    rt_timer_t    srctimer;
    rt_timer_t    subctrltimer;

    rt_uint8_t *fb;
    rt_uint32_t fblen;
#ifdef NOTE_TEST
    rt_uint8_t *savefb;
    rt_uint32_t savefblen;
#endif

    rt_uint8_t *ctrlfb;
    rt_uint32_t ctrlfblen;

    rt_event_t  disp_event;
    rt_uint32_t cmd;

    rt_uint8_t  btnprev;
    rt_uint8_t  btnsave;
    rt_uint8_t  btndel;
    rt_uint8_t  btnexit;
    rt_uint8_t  btnnext;

    rt_uint8_t  subctrlflag;
    rt_uint8_t  tools;
    rt_uint8_t  lineid;
    rt_uint8_t  rubid;

    rt_uint16_t touch_xlast;
    rt_uint16_t touch_ylast;

    image_info_t *pageinfo;
};

#define NOTE_LINE_MAX_NUM  8
static image_info_t *note_lines0[NOTE_LINE_MAX_NUM] =
{
    &note_subbtnline040_info,
    &note_subbtnline080_info,
    &note_subbtnline120_info,
    &note_subbtnline160_info,
    &note_subbtnline200_info,
    &note_subbtnline240_info,
    &note_subbtnline280_info,
    &note_subbtnline320_info,
};
static image_info_t *note_lines1[NOTE_LINE_MAX_NUM] =
{
    &note_subbtnline041_info,
    &note_subbtnline081_info,
    &note_subbtnline121_info,
    &note_subbtnline161_info,
    &note_subbtnline201_info,
    &note_subbtnline241_info,
    &note_subbtnline281_info,
    &note_subbtnline321_info,
};

#define NOTE_RUB_MAX_NUM  8
static image_info_t *note_rubs0[NOTE_RUB_MAX_NUM] =
{
    &note_subbtnrub040_info,
    &note_subbtnrub080_info,
    &note_subbtnrub120_info,
    &note_subbtnrub160_info,
    &note_subbtnrub200_info,
    &note_subbtnrub240_info,
    &note_subbtnrub280_info,
    &note_subbtnrub320_info,
};
static image_info_t *note_rubs1[NOTE_RUB_MAX_NUM] =
{
    &note_subbtnrub041_info,
    &note_subbtnrub081_info,
    &note_subbtnrub121_info,
    &note_subbtnrub161_info,
    &note_subbtnrub201_info,
    &note_subbtnrub241_info,
    &note_subbtnrub281_info,
    &note_subbtnrub321_info,
};

#define NOTE_DOT_MAX_NUM  8
static image_info_t *note_dots0[NOTE_DOT_MAX_NUM] =
{
    &note_dot040_info,
    &note_dot080_info,
    &note_dot120_info,
    &note_dot160_info,
    &note_dot200_info,
    &note_dot240_info,
    &note_dot280_info,
    &note_dot320_info,
};
static image_info_t *note_dots1[NOTE_DOT_MAX_NUM] =
{
    &note_dot041_info,
    &note_dot081_info,
    &note_dot121_info,
    &note_dot161_info,
    &note_dot201_info,
    &note_dot241_info,
    &note_dot281_info,
    &note_dot321_info,
};

struct olpc_note_touch_point
{
    rt_uint16_t x;
    rt_uint16_t y;
};
#define NOTE_POINT_BUFFER_SIZE      32
struct olpc_note_touch_point note_point_buffer[NOTE_POINT_BUFFER_SIZE];

/*
 **************************************************************************************************
 *
 * 1bpp note test demo
 *
 **************************************************************************************************
 */

/**
 * olpc note lut set.
 */
static rt_err_t olpc_note_lutset(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut0, lut1;

    lut0.winId = NOTE_TEXT_GRAY1_WIN;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    lut0.lut  = bpp1_lut;
    lut0.size = sizeof(bpp1_lut) / sizeof(bpp1_lut[0]);

    lut1.winId = NOTE_CTRL_RGB332_WIN;
    lut1.format = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    lut1.lut  = bpp_lut;
    lut1.size = sizeof(bpp_lut) / sizeof(bpp_lut[0]);

    ret = rt_display_lutset(&lut0, &lut1, RT_NULL);
    RT_ASSERT(ret == RT_EOK);

    // clear screen
    {
        struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;
        rt_device_t device = olpc_data->disp->device;
        struct rt_device_graphic_info info;

        ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
        RT_ASSERT(ret == RT_EOK);

        rt_display_win_clear(NOTE_CTRL_RGB332_WIN, RTGRAPHIC_PIXEL_FORMAT_RGB332, 0, WIN_LAYERS_H, 0);
    }

    return ret;
}

/**
 * olpc note demo init.
 */
static rt_err_t olpc_note_init(struct olpc_note_data *olpc_data)
{
    rt_err_t    ret;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->fblen = NOTE_DRAW_FB_W * NOTE_DRAW_FB_H / 8;
    olpc_data->fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->fblen);
    RT_ASSERT(olpc_data->fb != RT_NULL);
    rt_memset((void *)olpc_data->fb, 0x00, olpc_data->fblen);

#ifdef NOTE_TEST
    olpc_data->savefblen = olpc_data->fblen;
    olpc_data->savefb    = (rt_uint8_t *)rt_malloc_dtcm(olpc_data->savefblen);
    RT_ASSERT(olpc_data->savefb != RT_NULL);
    rt_memset((void *)olpc_data->savefb, 0x00, olpc_data->savefblen);

    save_item.type  = IMG_TYPE_RAW;
    save_item.pixel = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    save_item.x     = 0;
    save_item.y     = 0;
    save_item.w     = NOTE_DRAW_FB_W;
    save_item.h     = NOTE_DRAW_FB_H;
    save_item.data  = olpc_data->savefb;
#endif

    olpc_data->ctrlfblen = NOTE_CTRL_FB_W * NOTE_CTRL_FB_H;
    olpc_data->ctrlfb    = (rt_uint8_t *)rt_malloc_large(olpc_data->ctrlfblen);
    RT_ASSERT(olpc_data->ctrlfb != RT_NULL);
    rt_memset((void *)olpc_data->ctrlfb, 0x00, olpc_data->ctrlfblen);

    return RT_EOK;
}

/**
 * olpc note demo deinit.
 */
static void olpc_note_deinit(struct olpc_note_data *olpc_data)
{
    rt_free_large((void *)olpc_data->ctrlfb);
    olpc_data->ctrlfb = RT_NULL;

    rt_free_large((void *)olpc_data->fb);
    olpc_data->fb = RT_NULL;

#ifdef NOTE_TEST
    rt_free_dtcm((void *)olpc_data->savefb);
    olpc_data->savefb = RT_NULL;
#endif
}

/**
 * olpc note control refresh
 */
static rt_err_t olpc_note_ctrl_region_refresh(struct olpc_note_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int32_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = NOTE_CTRL_RGB332_WIN;
    wincfg->fb    = olpc_data->ctrlfb;
    wincfg->w     = ((NOTE_CTRL_FB_W + 3) / 4) * 4;
    wincfg->h     = NOTE_CTRL_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h;
    wincfg->x     = NOTE_CTRL_REGION_X + (NOTE_CTRL_REGION_W - NOTE_CTRL_FB_W) / 2;
    wincfg->y     = NOTE_CTRL_REGION_Y + (NOTE_CTRL_REGION_H - NOTE_CTRL_FB_H) / 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    rt_memset(wincfg->fb, 0, wincfg->fblen);

    /* prev button */
    {
        img_info = &note_btnprev0_info;
        if (olpc_data->btnprev)
        {
            img_info = &note_btnprev1_info;
        }
        xoffset = 0;
        yoffset = 0;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
    }
    /* line button */
    {
        img_info = &note_btnline0_info;
        if (olpc_data->tools == 0)
        {
            img_info = &note_btnline1_info;
        }
        xoffset = 0;
        yoffset = 0;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
    }
    /* rub button */
    {
        img_info = &note_btnrub0_info;
        if (olpc_data->tools == 1)
        {
            img_info = &note_btnrub1_info;
        }
        xoffset = 0;
        yoffset = 0;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
    }
    /* save button */
    {
        img_info = &note_btnsave0_info;
        if (olpc_data->btnsave)
        {
            img_info = &note_btnsave1_info;
        }
        xoffset = 0;
        yoffset = 0;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
    }
    /* del button */
    {
        img_info = &note_btndel0_info;
        if (olpc_data->btndel)
        {
            img_info = &note_btndel1_info;
        }
        xoffset = 0;
        yoffset = 0;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
    }
    /* exit button */
    {
        img_info = &note_btnexit0_info;
        if (olpc_data->btnexit)
        {
            img_info = &note_btnexit1_info;
        }
        xoffset = 0;
        yoffset = 0;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
    }
    /* next button */
    {
        img_info = &note_btnnext0_info;
        if (olpc_data->btnnext)
        {
            img_info = &note_btnnext1_info;
        }
        xoffset = 0;
        yoffset = 0;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
    }

    return RT_EOK;
}

/**
 * olpc note subcontrol refresh
 */
static rt_err_t olpc_note_subctrl_region_refresh(struct olpc_note_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int32_t   i, xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = NOTE_CTRL_RGB332_WIN;
    wincfg->fb    = olpc_data->ctrlfb;
    wincfg->w     = ((NOTE_CTRL1_FB_W + 3) / 4) * 4;
    wincfg->h     = NOTE_CTRL1_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h;
    wincfg->x     = NOTE_CTRL1_REGION_X + (NOTE_CTRL1_REGION_W - NOTE_CTRL1_FB_W) / 2;
    wincfg->y     = NOTE_CTRL1_REGION_Y + (NOTE_CTRL1_REGION_H - NOTE_CTRL1_FB_H) / 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    rt_memset(wincfg->fb, 0x6d, wincfg->fblen);

    if (olpc_data->tools == 0)
    {
        /* display line subcontrol */
        xoffset = 0;
        yoffset = 0;
        for (i = 0; i < NOTE_LINE_MAX_NUM; i++)
        {
            img_info = note_lines0[i];
            if (olpc_data->lineid == i)
            {
                img_info = note_lines1[i];
            }
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
        }
    }
    else
    {
        /* display rubber subcontrol */
        xoffset = 0;
        yoffset = 0;
        for (i = 0; i < NOTE_RUB_MAX_NUM; i++)
        {
            img_info = note_rubs0[i];
            if (olpc_data->rubid == i)
            {
                img_info = note_rubs1[i];
            }
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
        }
    }

    return RT_EOK;
}

/**
 * olpc note refresh draw
 */
static rt_err_t olpc_note_draw_region_refresh(struct olpc_note_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int32_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = NOTE_TEXT_GRAY1_WIN;
    wincfg->x     = NOTE_DRAW_REGION_X + (NOTE_DRAW_REGION_W - NOTE_DRAW_FB_W) / 2;
    wincfg->y     = NOTE_DRAW_REGION_Y + (NOTE_DRAW_REGION_H - NOTE_DRAW_FB_H) / 2;
    wincfg->w     = ((NOTE_DRAW_FB_W + 31) / 32) * 32;
    wincfg->h     = NOTE_DRAW_FB_H;
    wincfg->fb    = olpc_data->fb;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->ylast = wincfg->y;
    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    /* img clear */
    if ((olpc_data->cmd & UPDATE_DRAW_CLEAR) == UPDATE_DRAW_CLEAR)
    {
        olpc_data->cmd &= ~UPDATE_DRAW_CLEAR;

        rt_memset(wincfg->fb, 0, wincfg->fblen);
    }
    /* img update */
    else if ((olpc_data->cmd & UPDATE_DRAW_IMG) == UPDATE_DRAW_IMG)
    {
        olpc_data->cmd &= ~UPDATE_DRAW_IMG;

        rt_int32_t i, j;
        rt_int16_t touch_x, touch_y;
        struct olpc_note_touch_point pointbuffer[NOTE_POINT_BUFFER_SIZE];

        rt_memcpy(pointbuffer, note_point_buffer, NOTE_POINT_BUFFER_SIZE * sizeof(struct olpc_note_touch_point));
        rt_memset(note_point_buffer, 0, NOTE_POINT_BUFFER_SIZE * sizeof(struct olpc_note_touch_point));

        img_info = note_dots0[olpc_data->lineid];
        if (olpc_data->tools == 1)
        {
            img_info = note_dots1[olpc_data->rubid];
        }
        for (j = 0 ; j < NOTE_POINT_BUFFER_SIZE; j++)
        {
            if ((pointbuffer[j].x == 0) && (pointbuffer[j].y == 0))
            {
                break;
            }

            touch_x = pointbuffer[j].x;
            touch_y = pointbuffer[j].y;

            rt_int32_t len   = MAX(1, MAX(ABS(touch_x - olpc_data->touch_xlast), ABS(touch_y - olpc_data->touch_ylast)));
            rt_int32_t xrate = ((touch_x - olpc_data->touch_xlast) * 100) / len;
            rt_int32_t yrate = ((touch_y - olpc_data->touch_ylast) * 100) / len;

            for (i = 0; i < len; i += MAX(1, img_info->w / 4))
            {
                xoffset = i * xrate / 100 + olpc_data->touch_xlast - wincfg->x - img_info->w / 2;
                yoffset = i * yrate / 100 + olpc_data->touch_ylast - wincfg->y - img_info->h / 2;
                rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
            }

            olpc_data->touch_xlast = touch_x;
            olpc_data->touch_ylast = touch_y;
        }
    }
    /* img review */
    else if ((olpc_data->cmd & UPDATE_REVIEW_IMG) == UPDATE_REVIEW_IMG)
    {
        olpc_data->cmd &= ~UPDATE_REVIEW_IMG;

        img_info = olpc_data->pageinfo;
        xoffset = 0;
        yoffset = 0;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, img_info->x + xoffset, img_info->y + yoffset);
    }

    return RT_EOK;
}

/**
 * olpc note refresh process
 */
static rt_err_t olpc_note_task_fun(struct olpc_note_data *olpc_data)
{
    rt_err_t ret;
    struct rt_display_config  wincfg0, wincfg1;
    struct rt_display_config *winhead = RT_NULL;

    rt_memset(&wincfg0, 0, sizeof(struct rt_display_config));
    rt_memset(&wincfg1, 0, sizeof(struct rt_display_config));
    wincfg0.zpos = WIN_BOTTOM_LAYER;
    wincfg1.zpos = WIN_TOP_LAYER;

    if ((olpc_data->cmd & UPDATE_DRAW) == UPDATE_DRAW)
    {
        olpc_data->cmd &= ~UPDATE_DRAW;

        olpc_note_draw_region_refresh(olpc_data, &wincfg0);
        rt_display_win_layers_list(&winhead, &wincfg0);
    }

    if ((olpc_data->cmd & UPDATE_CTRL) == UPDATE_CTRL)
    {
        olpc_data->cmd &= ~UPDATE_CTRL;

        olpc_note_ctrl_region_refresh(olpc_data, &wincfg1);
        rt_display_win_layers_list(&winhead, &wincfg1);
    }
    else if ((olpc_data->cmd & UPDATE_SUBCTRL) == UPDATE_SUBCTRL)
    {
        olpc_data->cmd &= ~UPDATE_SUBCTRL;

        olpc_note_subctrl_region_refresh(olpc_data, &wincfg1);
        rt_display_win_layers_list(&winhead, &wincfg1);
    }

    //refresh screen
    ret = rt_display_win_layers_set(winhead);
    RT_ASSERT(ret == RT_EOK);

    if (olpc_data->cmd != 0)
    {
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
    }

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * olpc note touch functions
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)
/**
 * screen touch.
 */
static rt_err_t olpc_note_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    default:
        break;
    }

#if defined(OLPC_APP_SRCSAVER_ENABLE)
    if ((olpc_data) && (olpc_data->srctimer))
    {
        rt_timer_start(olpc_data->srctimer);    //reset screen protection timer
    }
#endif

    return ret;
}

static rt_err_t olpc_note_screen_touch_register(void *parameter)
{
    image_info_t *img_info = &screen_item;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));
    rt_memset(note_point_buffer, 0, NOTE_POINT_BUFFER_SIZE * sizeof(struct olpc_note_touch_point));

    /* screen on button touch register */
    {
        img_info->w = WIN_SCALED_W;
        img_info->h = WIN_SCALED_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_note_screen_touch_unregister(void *parameter)
{
    image_info_t *img_info = &screen_item;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));
    return RT_EOK;
}

/**
 * draw touch.
 */
static rt_err_t olpc_note_draw_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    rt_uint32_t i;
    image_info_t *img_info;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;
    struct rt_device_graphic_info info;
    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    if (olpc_data->subctrlflag != 0)
    {
        if (point->y <= NOTE_CTRL1_REGION_Y + NOTE_CTRL1_REGION_H)
        {
            return ret;
        }
        else
        {
            if (olpc_data->subctrlflag == 1)
            {
                olpc_note_subbtnline_touch_unregister(olpc_data);
            }
            else if (olpc_data->subctrlflag == 2)
            {
                olpc_note_subbtnrub_touch_unregister(olpc_data);
            }

            olpc_data->subctrlflag = 0;

            olpc_data->cmd = UPDATE_DRAW | UPDATE_DRAW_IMG;
            rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);

            rt_timer_stop(olpc_data->subctrltimer);
        }
    }

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->touch_xlast = point->x;
        olpc_data->touch_ylast = point->y;
    case TOUCH_EVENT_MOVE:
        img_info = note_dots0[olpc_data->lineid];
        if (olpc_data->tools == 1)
        {
            img_info = note_dots1[olpc_data->rubid];
        }
        if (((NOTE_DRAW_REGION_X + img_info->w <= point->x) && (point->x < NOTE_DRAW_REGION_X + NOTE_DRAW_REGION_W - img_info->w)) &&
                ((NOTE_DRAW_REGION_Y + img_info->h <= point->y) && (point->y < NOTE_DRAW_REGION_Y + NOTE_DRAW_REGION_H - img_info->h)))
        {
            for (i = 0 ; i < NOTE_POINT_BUFFER_SIZE; i++)
            {
                if ((note_point_buffer[i].x == 0) && (note_point_buffer[i].y == 0))
                {
                    break;
                }
            }
            if (i < NOTE_POINT_BUFFER_SIZE)
            {
                note_point_buffer[i].x = point->x;
                note_point_buffer[i].y = point->y;
                olpc_data->cmd = UPDATE_DRAW | UPDATE_DRAW_IMG;
                rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
            }
            else
            {
                rt_kprintf("olpc note touch point overflow!!!\n");
            }
        }
        else
        {
            olpc_touch_reset();
        }
        break;

    default:
        break;
    }

    return ret;
}

static rt_err_t olpc_note_draw_touch_register(void *parameter)
{
    image_info_t *img_info = &draw_item;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button touch register */
    {
        img_info->x = NOTE_DRAW_REGION_X;
        img_info->y = NOTE_DRAW_REGION_Y;
        img_info->w = NOTE_DRAW_REGION_W;
        img_info->h = NOTE_DRAW_REGION_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_draw_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_note_draw_touch_unregister(void *parameter)
{
    image_info_t *img_info = &draw_item;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));
    return RT_EOK;
}

/**
 * prev btn touch.
 */
static rt_err_t olpc_note_btnprev_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnprev = 1;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnprev = 0;
#ifdef NOTE_TEST
        olpc_data->pageinfo = &note_prevpage_info;
        olpc_data->cmd |= UPDATE_DRAW | UPDATE_REVIEW_IMG;
#endif
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    default:
        break;
    }
    return ret;
}

/**
 * next btn touch.
 */
static rt_err_t olpc_note_btnnext_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnnext = 1;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnnext = 0;
#ifdef NOTE_TEST
        olpc_data->pageinfo = &save_item;
        olpc_data->cmd |= UPDATE_DRAW | UPDATE_REVIEW_IMG;
#endif
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    default:
        break;
    }
    return ret;
}

/**
 * line btn touch.
 */
static rt_err_t olpc_note_btnline_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->tools = 0;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);

        olpc_data->subctrlflag = 1;
        olpc_note_subbtnline_touch_register(olpc_data);

        rt_timer_start(olpc_data->subctrltimer);
        break;

    default:
        break;
    }
    return ret;
}

/**
 * rub btn touch.
 */
static rt_err_t olpc_note_btnrub_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->tools = 1;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);

        olpc_data->subctrlflag = 2;
        olpc_note_subbtnrub_touch_register(olpc_data);

        rt_timer_start(olpc_data->subctrltimer);
        break;

    default:
        break;
    }
    return ret;
}

/**
 * save btn touch.
 */
static rt_err_t olpc_note_btnsave_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnsave = 1;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnsave = 0;
#ifdef NOTE_TEST
        rt_memcpy(olpc_data->savefb, olpc_data->fb, olpc_data->fblen);
#endif
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;
    default:
        break;
    }
    return ret;
}

/**
 * del btn touch.
 */
static rt_err_t olpc_note_btndel_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btndel = 1;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btndel = 0;
#ifdef NOTE_TEST
        rt_memset(olpc_data->savefb, 0x00, olpc_data->savefblen);
#endif
        olpc_data->cmd |= UPDATE_DRAW | UPDATE_DRAW_CLEAR;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    default:
        break;
    }
    return ret;
}

/**
 * exit btn touch.
 */
static rt_err_t olpc_note_btnexit_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->btnexit = 1;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        break;

    case TOUCH_EVENT_UP:
        olpc_data->btnexit = 0;
        olpc_data->cmd |= UPDATE_CTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);

        rt_thread_delay(10);
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_EXIT);
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_btn_touch_register(void *parameter)
{
    rt_int16_t   xoffset = 0;
    rt_int16_t   yoffset = 0;
    image_info_t *img_info;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));
    xoffset = NOTE_CTRL_REGION_X + (NOTE_CTRL_REGION_W - NOTE_CTRL_FB_W) / 2;
    yoffset = NOTE_CTRL_REGION_Y + (NOTE_CTRL_REGION_H - NOTE_CTRL_FB_H) / 2;

    /* prev button touch register */
    img_info = &note_btnprev0_info;
    {
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_btnprev_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);
    }
    /* line button touch register */
    img_info = &note_btnline0_info;
    {
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_btnline_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);
    }
    /* rubber button touch register */
    img_info = &note_btnrub0_info;
    {
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_btnrub_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);
    }
    /* save button touch register */
    img_info = &note_btnsave0_info;
    {
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_btnsave_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);
    }
    /* del button touch register */
    img_info = &note_btndel0_info;
    {
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_btndel_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);
    }
    /* exit button touch register */
    img_info = &note_btnexit0_info;
    {
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_btnexit_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);
    }
    /* next button touch register */
    img_info = &note_btnnext0_info;
    {
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_btnnext_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);
    }

    return RT_EOK;
}

static rt_err_t olpc_note_btn_touch_unregister(void *parameter)
{
    image_info_t *img_info;

    img_info = &note_btnprev0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_btnnext0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_btnline0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_btnrub0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_btnsave0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_btndel0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_btnexit0_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

static rt_err_t olpc_note_touch_register(void *parameter)
{
    olpc_note_screen_touch_register(parameter);
    olpc_note_draw_touch_register(parameter);
    olpc_note_btn_touch_register(parameter);
    return RT_EOK;
}

static rt_err_t olpc_note_touch_unregister(void *parameter)
{
    olpc_note_btn_touch_unregister(parameter);
    olpc_note_draw_touch_unregister(parameter);
    olpc_note_screen_touch_unregister(parameter);
    return RT_EOK;
}

/* line selete */
static rt_err_t olpc_note_subbtnline04_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->lineid = 0;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnline08_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->lineid = 1;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnline12_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->lineid = 2;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnline16_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->lineid = 3;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnline20_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->lineid = 4;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnline24_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->lineid = 5;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnline28_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->lineid = 6;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnline32_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->lineid = 7;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnline_touch_register(void *parameter)
{
    rt_int16_t   xoffset = 0;
    rt_int16_t   yoffset = 0;
    image_info_t *img_info;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));
    xoffset = NOTE_CTRL1_REGION_X + (NOTE_CTRL1_REGION_W - NOTE_CTRL1_FB_W) / 2;
    yoffset = NOTE_CTRL1_REGION_Y + (NOTE_CTRL1_REGION_H - NOTE_CTRL1_FB_H) / 2;

    img_info = &note_subbtnline040_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnline04_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnline080_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnline08_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnline120_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnline12_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnline160_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnline16_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnline200_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnline20_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnline240_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnline24_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnline280_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnline28_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnline320_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnline32_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    return RT_EOK;
}

static rt_err_t olpc_note_subbtnline_touch_unregister(void *parameter)
{
    image_info_t *img_info;

    img_info = &note_subbtnline040_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnline080_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnline120_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnline160_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnline200_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnline240_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnline280_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnline320_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

/* rubber select */
static rt_err_t olpc_note_subbtnrub04_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->rubid = 0;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnrub08_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->rubid = 1;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnrub12_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->rubid = 2;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnrub16_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->rubid = 3;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnrub20_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->rubid = 4;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnrub24_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->rubid = 5;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnrub28_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->rubid = 6;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

static rt_err_t olpc_note_subbtnrub32_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->rubid = 7;
        olpc_data->cmd |= UPDATE_SUBCTRL;
        rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
        rt_timer_start(olpc_data->subctrltimer);  //restart timer
        break;
    default:
        break;
    }
    return ret;
}

/* rub selete */
static rt_err_t olpc_note_subbtnrub_touch_register(void *parameter)
{
    rt_int16_t   xoffset = 0;
    rt_int16_t   yoffset = 0;
    image_info_t *img_info;
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));
    xoffset = NOTE_CTRL1_REGION_X + (NOTE_CTRL1_REGION_W - NOTE_CTRL1_FB_W) / 2;
    yoffset = NOTE_CTRL1_REGION_Y + (NOTE_CTRL1_REGION_H - NOTE_CTRL1_FB_H) / 2;

    img_info = &note_subbtnrub040_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnrub04_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnrub080_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnrub08_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnrub120_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnrub12_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnrub160_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnrub16_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnrub200_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnrub20_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnrub240_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnrub24_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnrub280_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnrub28_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    img_info = &note_subbtnrub320_info;
    register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_note_subbtnrub32_touch_callback, (void *)olpc_data, 0);
    update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), img_info->x, img_info->y, xoffset, yoffset);

    return RT_EOK;
}

static rt_err_t olpc_note_subbtnrub_touch_unregister(void *parameter)
{
    image_info_t *img_info;

    img_info = &note_subbtnrub040_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnrub080_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnrub120_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnrub160_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnrub200_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnrub240_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnrub280_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    img_info = &note_subbtnrub320_info;
    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

static void olpc_note_subctrltmr_callback(void *parameter)
{
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    if (olpc_data->subctrlflag == 1)
    {
        olpc_note_subbtnline_touch_unregister(olpc_data);
    }
    else if (olpc_data->subctrlflag == 2)
    {
        olpc_note_subbtnrub_touch_unregister(olpc_data);
    }

    olpc_data->subctrlflag = 0;

    olpc_data->cmd = UPDATE_DRAW | UPDATE_DRAW_IMG;
    rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);
}

#endif

/*
 **************************************************************************************************
 *
 * olpc note screen protection functions
 *
 **************************************************************************************************
 */
#if defined(OLPC_APP_SRCSAVER_ENABLE)

/**
 * note screen protection timeout callback.
 */
static void olpc_note_srcprotect_callback(void *parameter)
{
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;
    rt_event_send(olpc_data->disp_event, EVENT_SRCSAVER_ENTER);
}

/**
 * exit screen protection hook.
 */
static rt_err_t olpc_note_screen_protection_hook(void *parameter)
{
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    rt_event_send(olpc_data->disp_event, EVENT_SRCSAVER_EXIT);

    return RT_EOK;
}

/**
 * start screen protection.
 */
static rt_err_t olpc_note_screen_protection_enter(void *parameter)
{
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    rt_timer_stop(olpc_data->srctimer);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_note_touch_unregister(parameter);
    olpc_touch_list_clear();
#endif

    // register exit hook & start screen protection
    olpc_srcprotect_app_init(olpc_note_screen_protection_hook, parameter);

    return RT_EOK;
}

/**
 * exit screen protection.
 */
static rt_err_t olpc_note_screen_protection_exit(void *parameter)
{
    struct olpc_note_data *olpc_data = (struct olpc_note_data *)parameter;

    olpc_note_lutset(olpc_data);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_note_touch_register(parameter);
#endif

    olpc_data->cmd  = UPDATE_DRAW;
    olpc_data->cmd |= UPDATE_CTRL;
    rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);

    rt_timer_start(olpc_data->srctimer);

    return RT_EOK;
}
#endif

/*
 **************************************************************************************************
 *
 * olpc note demo init & thread
 *
 **************************************************************************************************
 */

/**
 * olpc note dmeo thread.
 */
static void olpc_note_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_note_data *olpc_data;

    olpc_data = (struct olpc_note_data *)rt_malloc(sizeof(struct olpc_note_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_note_data));

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_note_lutset(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_note_touch_register(olpc_data);
    olpc_data->subctrltimer = rt_timer_create("notesub",
                              olpc_note_subctrltmr_callback,
                              (void *)olpc_data,
                              NOTE_SUBCTRL_TIME,
                              RT_TIMER_FLAG_ONE_SHOT);
    RT_ASSERT(olpc_data->subctrltimer != RT_NULL);
#endif

    olpc_data->disp_event = rt_event_create("display_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->disp_event != RT_NULL);

    ret = olpc_note_init(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
    olpc_data->srctimer = rt_timer_create("noteprotect",
                                          olpc_note_srcprotect_callback,
                                          (void *)olpc_data,
                                          NOTE_SRCSAVER_TIME,
                                          RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->srctimer != RT_NULL);

    rt_timer_start(olpc_data->srctimer);
#endif

    olpc_data->cmd  = UPDATE_DRAW | UPDATE_DRAW_CLEAR;
    olpc_data->cmd |= UPDATE_CTRL;
    rt_event_send(olpc_data->disp_event, EVENT_NOTE_REFRESH);

    while (1)
    {
        ret = rt_event_recv(olpc_data->disp_event,
                            EVENT_NOTE_REFRESH | EVENT_NOTE_EXIT | EVENT_SRCSAVER_ENTER | EVENT_SRCSAVER_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_NOTE_REFRESH)
        {
            ret = olpc_note_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        if (event & EVENT_SRCSAVER_ENTER)
        {
            ret = olpc_note_screen_protection_enter(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_SRCSAVER_EXIT)
        {
            ret = olpc_note_screen_protection_exit(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }
#endif
        if (event & EVENT_NOTE_EXIT)
        {
            break;
        }
    }

    /* Thread deinit */
#if defined(OLPC_APP_SRCSAVER_ENABLE)
    rt_timer_stop(olpc_data->srctimer);
    ret = rt_timer_delete(olpc_data->srctimer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->srctimer = RT_NULL;
#endif

#if defined(RT_USING_PISCES_TOUCH)
    rt_timer_stop(olpc_data->subctrltimer);
    ret = rt_timer_delete(olpc_data->subctrltimer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->subctrltimer = RT_NULL;

    olpc_note_touch_unregister(olpc_data);
    olpc_touch_list_clear();
#endif

    olpc_note_deinit(olpc_data);

    rt_event_delete(olpc_data->disp_event);
    olpc_data->disp_event = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;

    rt_event_send(olpc_main_event, EVENT_APP_CLOCK);
}

/**
 * olpc note demo application init.
 */
#if defined(OLPC_DLMODULE_ENABLE)
SECTION(".param") rt_uint16_t dlmodule_thread_priority = 5;
SECTION(".param") rt_uint32_t dlmodule_thread_stacksize = 2048;
int main(int argc, char *argv[])
{
    olpc_note_thread(RT_NULL);
    return RT_EOK;
}

#else
int olpc_note_app_init(void)
{
    rt_thread_t rtt_note;

    rtt_note = rt_thread_create("olpcnote", olpc_note_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(rtt_note != RT_NULL);
    rt_thread_startup(rtt_note);

    return RT_EOK;
}
//INIT_APP_EXPORT(olpc_note_app_init);
#endif

#endif
