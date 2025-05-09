/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_SRCSAVER_ENABLE)

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
    0x000000, 0x00FFFFFF
};

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */
#define SCRPROTECTION_GRAY1_WIN      0

#define SCRPROTECTION_REGION_X       0
#define SCRPROTECTION_REGION_Y       0
#define SCRPROTECTION_REGION_W       WIN_LAYERS_W
#define SCRPROTECTION_REGION_H       WIN_LAYERS_H
#define SCRPROTECTION_FB_W           (320 + 32)
#define SCRPROTECTION_FB_H           240

/* Event define */
#define EVENT_SCRPROTECTION_START    (0x01UL << 0)
#define EVENT_SCRPROTECTION_REFRESH  (0x01UL << 1)
#define EVENT_SCRPROTECTION_EXIT     (0x01UL << 2)

/* Screen protection state*/
#define STATE_SCRPROTECTION_STOP     0
#define STATE_SCRPROTECTION_RUN      (0x01UL << 0)

/* Command define */
#define CMD_ANIMAL_UPDATE           (0x01UL << 0)

#define SCRPROTECTION_REFRESH_TIME   (RT_TICK_PER_SECOND * 1)

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern image_info_t screen_dog0_info;
extern image_info_t screen_dog1_info;
extern image_info_t screen_dog2_info;
extern image_info_t screen_dog3_info;
extern image_info_t screen_dog4_info;
extern image_info_t screen_dog5_info;
extern image_info_t screen_dog6_info;

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_srcprotect_data
{
    rt_display_data_t disp;
    rt_timer_t        timer;

    rt_uint8_t *fb;
    rt_uint32_t fblen;

    rt_event_t  disp_event;
    rt_uint32_t cmd;
    rt_uint32_t state;

    rt_uint16_t picid;
    rt_int16_t  pic_ylast;

    rt_int16_t  pic_x;
    rt_int16_t  pic_y;

    rt_err_t (*exithook)(void *parameter);
    void *exithookparam;
};
static struct olpc_srcprotect_data *g_srcprotect_data = RT_NULL;

#define SCRPROTECTION_PIC_MAX_NUM  7
static image_info_t *srcprotect_pic_num[SCRPROTECTION_PIC_MAX_NUM] =
{
    &screen_dog0_info,
    &screen_dog1_info,
    &screen_dog2_info,
    &screen_dog3_info,
    &screen_dog4_info,
    &screen_dog5_info,
    &screen_dog6_info,
};

/*
 **************************************************************************************************
 *
 * screen protection demo
 *
 **************************************************************************************************
 */
/**
 * screen protection timer callback.
 */
static void olpc_srcprotect_timer_callback(void *parameter)
{
    struct olpc_srcprotect_data *olpc_data = (struct olpc_srcprotect_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    image_info_t *img_info = srcprotect_pic_num[0];

    if (++olpc_data->picid >= SCRPROTECTION_PIC_MAX_NUM)
    {
        olpc_data->picid = 0;

        olpc_data->pic_y += img_info->h;
        if (olpc_data->pic_y > WIN_LAYERS_H - img_info->h)
        {
            olpc_data->pic_y = 0;
        }
    }
    olpc_data->pic_x = olpc_data->picid * 96 + 44;

    olpc_data->cmd |= CMD_ANIMAL_UPDATE;
    rt_event_send(olpc_data->disp_event, EVENT_SCRPROTECTION_REFRESH);
}

/**
 * screen protection refresh process
 */
static rt_err_t olpc_srcprotect_animal_region_refresh(struct olpc_srcprotect_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int32_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = SCRPROTECTION_GRAY1_WIN;
    wincfg->fb    = olpc_data->fb;
    wincfg->x     = ((SCRPROTECTION_REGION_X + olpc_data->pic_x) / 32) * 32;
    wincfg->y     = ((SCRPROTECTION_REGION_Y + olpc_data->pic_y) /  2) * 2;
    wincfg->w     = ((SCRPROTECTION_FB_W + 31) / 32) * 32;
    wincfg->h     = SCRPROTECTION_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->ylast = olpc_data->pic_ylast;
    olpc_data->pic_ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    rt_memset(wincfg->fb, 0, wincfg->fblen);
    {
        img_info = srcprotect_pic_num[olpc_data->picid];
        xoffset = olpc_data->pic_x % 32;
        yoffset = 0;
        RT_ASSERT(img_info->w <= wincfg->w);
        RT_ASSERT(img_info->h <= wincfg->h);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    return RT_EOK;
}


/**
 * screen protection refresh process
 */
static rt_err_t olpc_srcprotect_task_fun(struct olpc_srcprotect_data *olpc_data)
{
    rt_err_t ret;
    struct rt_display_config  wincfg;
    struct rt_display_config *winhead = RT_NULL;

    rt_memset(&wincfg, 0, sizeof(struct rt_display_config));

    if ((olpc_data->cmd & CMD_ANIMAL_UPDATE) == CMD_ANIMAL_UPDATE)
    {
        olpc_data->cmd &= ~CMD_ANIMAL_UPDATE;
        olpc_srcprotect_animal_region_refresh(olpc_data, &wincfg);
    }

    //refresh screen
    rt_display_win_layers_list(&winhead, &wincfg);
    ret = rt_display_win_layers_set(winhead);
    RT_ASSERT(ret == RT_EOK);

    if (olpc_data->cmd != 0)
    {
        rt_event_send(olpc_data->disp_event, EVENT_SCRPROTECTION_REFRESH);
    }

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * screen protection touch functions
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)
/**
 * screen touch.
 */
static image_info_t screen_item;
static rt_err_t olpc_srcprotect_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_srcprotect_data *olpc_data = (struct olpc_srcprotect_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_UP:
        rt_event_send(olpc_data->disp_event, EVENT_SCRPROTECTION_EXIT);
        break;

    default:
        break;
    }

    return ret;
}

static rt_err_t olpc_srcprotect_screen_touch_register(void *parameter)
{
    image_info_t *img_info = &screen_item;
    struct olpc_srcprotect_data *olpc_data = (struct olpc_srcprotect_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button touch register */
    {
        screen_item.w = WIN_LAYERS_W;
        screen_item.h = WIN_LAYERS_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_srcprotect_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_srcprotect_screen_touch_unregister(void *parameter)
{
    image_info_t *img_info = &screen_item;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}
#endif

/*
 **************************************************************************************************
 *
 * screen protection init & thread
 *
 **************************************************************************************************
 */

/**
 * olpc note lut set.
 */
static rt_err_t olpc_srcprotect_lutset(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut0;

    lut0.winId = SCRPROTECTION_GRAY1_WIN;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    lut0.lut  = bpp1_lut;
    lut0.size = sizeof(bpp1_lut) / sizeof(bpp1_lut[0]);

    ret = rt_display_lutset(&lut0, RT_NULL, RT_NULL);
    RT_ASSERT(ret == RT_EOK);

    // clear screen
    {
        struct olpc_srcprotect_data *olpc_data = (struct olpc_srcprotect_data *)parameter;
        rt_device_t device = olpc_data->disp->device;
        struct rt_device_graphic_info info;

        ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
        RT_ASSERT(ret == RT_EOK);

        rt_display_win_clear(SCRPROTECTION_GRAY1_WIN, RTGRAPHIC_PIXEL_FORMAT_GRAY1, 0, WIN_LAYERS_H, 0);
    }

    return ret;
}

/**
 * screen protection thread.
 */
static void olpc_srcprotect_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_srcprotect_data *olpc_data;
    struct rt_device_graphic_info info;

    olpc_data = (struct olpc_srcprotect_data *)rt_malloc(sizeof(struct olpc_srcprotect_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_srcprotect_data));
    g_srcprotect_data = olpc_data;

    olpc_data->picid = 0;
    olpc_data->state = STATE_SCRPROTECTION_STOP;

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_srcprotect_lutset(olpc_data);
    RT_ASSERT(ret == RT_EOK);

    rt_device_t  device = olpc_data->disp->device;
    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->fblen = SCRPROTECTION_FB_W * SCRPROTECTION_FB_H / 8;
    olpc_data->fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->fblen);
    RT_ASSERT(olpc_data->fb != RT_NULL);
    rt_memset(olpc_data->fb, 0, olpc_data->fblen);

    //olpc_srcprotect_srcclear(olpc_data);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_srcprotect_screen_touch_register(olpc_data);
#endif

    olpc_data->disp_event = rt_event_create("srcprotect", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->disp_event != RT_NULL);

    olpc_data->timer = rt_timer_create("srcprotect",
                                       olpc_srcprotect_timer_callback,
                                       (void *)olpc_data,
                                       SCRPROTECTION_REFRESH_TIME,
                                       RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->timer != RT_NULL);
    rt_timer_start(olpc_data->timer);

    olpc_data->cmd   = CMD_ANIMAL_UPDATE;
    rt_event_send(olpc_data->disp_event, EVENT_SCRPROTECTION_START | EVENT_SCRPROTECTION_REFRESH);

    while (1)
    {
        ret = rt_event_recv(olpc_data->disp_event, EVENT_SCRPROTECTION_START | EVENT_SCRPROTECTION_REFRESH | EVENT_SCRPROTECTION_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_SCRPROTECTION_REFRESH)
        {
            ret = olpc_srcprotect_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_SCRPROTECTION_EXIT)
        {
            //olpc_srcprotect_srcclear(olpc_data);
            break;
        }
    }

    /* Thread deinit */
    rt_timer_stop(olpc_data->timer);
    ret = rt_timer_delete(olpc_data->timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->timer = RT_NULL;

#if defined(RT_USING_PISCES_TOUCH)
    olpc_srcprotect_screen_touch_unregister(olpc_data);
#endif

    rt_event_delete(olpc_data->disp_event);
    olpc_data->disp_event = RT_NULL;

    rt_free_large((void *)olpc_data->fb);
    olpc_data->fb = RT_NULL;

    if (olpc_data->exithook)
    {
        olpc_data->exithook(olpc_data->exithookparam);
    }

    rt_free((void *)olpc_data);
    olpc_data = RT_NULL;
}

/**
 * screen protection start API.
 */
void olpc_srcprotect_app_init(rt_err_t (*hook)(void *parameter), void *parameter)
{
    static rt_thread_t rtt_srcprotect;

    rtt_srcprotect = rt_thread_create("srcprotect", olpc_srcprotect_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(rtt_srcprotect != RT_NULL);

    rt_thread_startup(rtt_srcprotect);

    // wait for thread initied
    do
    {
        rt_thread_delay(1);
    }
    while (g_srcprotect_data == RT_NULL);

    g_srcprotect_data->exithook      = hook;
    g_srcprotect_data->exithookparam = parameter;
}
RTM_EXPORT(olpc_srcprotect_app_init);

#endif
