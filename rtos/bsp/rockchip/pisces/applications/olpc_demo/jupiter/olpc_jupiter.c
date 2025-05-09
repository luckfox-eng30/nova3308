/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_JUPITER_ENABLE)

#include "drv_heap.h"
#include "applications/common/image_info.h"
#include "applications/common/olpc_display.h"
#include "applications/common/olpc_ap.h"

#if defined(RT_USING_PISCES_TOUCH)
#include "drv_touch.h"
#include "applications/common/olpc_touch.h"
#endif

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */
#define JUPITER_RGB332_WIN        0
#define JUPITER_TIME_RGB332_WIN   1

#define JUPITER_REGION_X          0
#define JUPITER_REGION_Y          0
#define JUPITER_REGION_W          WIN_LAYERS_W
#define JUPITER_REGION_H          WIN_LAYERS_H
#define JUPITER_FB_W              500            /* jupiter frame buffer w */
#define JUPITER_FB_H              500            /* jupiter frame buffer h */

#define JUPITER_TIME_FB_W         200            /* jupiter time buffer w */
#define JUPITER_TIME_FB_H         300            /* jupiter time buffer h */

/* Event define */
#define EVENT_JUPITER_REFRESH     (0x01UL << 0)
#define EVENT_JUPITER_DISP        (0x01UL << 1)
#define EVENT_JUPITER_EXIT        (0x01UL << 2)

/* Command define */
#define UPDATE_JUPITER            (0x01UL << 0)

#define JUPITER_SRCSAVER_TIME     (RT_TICK_PER_SECOND / 30)

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern image_info_t jupiter_time_info;

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_jupiter_data
{
    rt_display_data_t disp;
    rt_timer_t        timer;

    rt_uint8_t *fb;
    rt_uint32_t fblen;

    rt_event_t  disp_event;
    rt_uint32_t cmd;

    rt_uint16_t pic_id;
};

static struct olpc_jupiter_data *g_olpc_data = RT_NULL;

#define JUPITER_PIC_INC_STEP 1
#define JUPITER_PIC_MAX_NUM  20
static image_info_t jupiter_pages_num[JUPITER_PIC_MAX_NUM];
static const char *jupiter_res_name[JUPITER_PIC_MAX_NUM] =
{
    "Jupiter00.lzw",
    "Jupiter01.lzw",
    "Jupiter02.lzw",
    "Jupiter03.lzw",
    "Jupiter04.lzw",
    "Jupiter05.lzw",
    "Jupiter06.lzw",
    "Jupiter07.lzw",
    "Jupiter08.lzw",
    "Jupiter09.lzw",
    "Jupiter10.lzw",
    "Jupiter11.lzw",
    "Jupiter12.lzw",
    "Jupiter13.lzw",
    "Jupiter14.lzw",
    "Jupiter15.lzw",
    "Jupiter16.lzw",
    "Jupiter17.lzw",
    "Jupiter18.lzw",
    "Jupiter19.lzw",
};

/*
 **************************************************************************************************
 *
 * jupiter test demo
 *
 **************************************************************************************************
 */

/**
 * olpc jupiter lut set.
 */
static rt_err_t olpc_jupiter_lutset(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut0, lut1;

    lut0.winId = JUPITER_RGB332_WIN;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    lut0.lut  = bpp_lut;
    lut0.size = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
    lut1.winId = JUPITER_TIME_RGB332_WIN;
    lut1.format = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    lut1.lut  = bpp_lut;
    lut1.size = sizeof(bpp_lut) / sizeof(bpp_lut[0]);

    ret = rt_display_lutset(&lut0, &lut1, RT_NULL);
    RT_ASSERT(ret == RT_EOK);

    // clear screen
    {
        struct olpc_jupiter_data *olpc_data = (struct olpc_jupiter_data *)parameter;
        rt_device_t device = olpc_data->disp->device;
        struct rt_device_graphic_info info;

        ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
        RT_ASSERT(ret == RT_EOK);

        rt_display_win_clear(JUPITER_RGB332_WIN, RTGRAPHIC_PIXEL_FORMAT_RGB332, 0, WIN_LAYERS_H, 0);
    }

    return ret;
}

/**
 * olpc jupiter demo init.
 */
static rt_err_t olpc_jupiter_init(struct olpc_jupiter_data *olpc_data)
{
    rt_err_t    ret;
    rt_uint32_t i;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->fblen = JUPITER_FB_W * JUPITER_FB_H;
    olpc_data->fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->fblen);
    RT_ASSERT(olpc_data->fb != RT_NULL);

    // load x-screen resource to memory
    {
        FILE_READ_REQ_PARAM Param;
        FILE_READ_REQ_PARAM *pParam = &Param;

        for (i = 0; i < JUPITER_PIC_MAX_NUM; i++)
        {
            rt_memset(pParam, 0, sizeof(FILE_READ_REQ_PARAM));

            /* Get dlmodule file size info */
            rt_strncpy(pParam->name, jupiter_res_name[i], rt_strlen(jupiter_res_name[i]));
            ret = olpc_ap_command(FILE_INFO_REQ, pParam, sizeof(FILE_READ_REQ_PARAM));
            RT_ASSERT(ret == RT_EOK);

            /* Malloc buffer for dlmodule file */
            pParam->buf = (rt_uint8_t *)rt_dma_malloc_dtcm(pParam->totalsize);
            RT_ASSERT(pParam->buf != RT_NULL);

            /* Read dlmodule file data */
            ret = olpc_ap_command(FIILE_READ_REQ, pParam, sizeof(FILE_READ_REQ_PARAM));
            RT_ASSERT(ret == RT_EOK);

            jupiter_pages_num[i].type  = IMG_TYPE_COMPRESS;
            jupiter_pages_num[i].pixel = RTGRAPHIC_PIXEL_FORMAT_RGB332;
            jupiter_pages_num[i].x = 0;
            jupiter_pages_num[i].y = 0;
            jupiter_pages_num[i].w = 500;
            jupiter_pages_num[i].h = 500;
            jupiter_pages_num[i].size = pParam->totalsize;
            jupiter_pages_num[i].data = pParam->buf;
        }
    }

    return RT_EOK;
}

/**
 * olpc jupiter demo deinit.
 */
static void olpc_jupiter_deinit(struct olpc_jupiter_data *olpc_data)
{
    rt_uint32_t i;

    for (i = 0; i < JUPITER_PIC_MAX_NUM; i++)
    {
        rt_dma_free_dtcm((rt_uint8_t *)jupiter_pages_num[i].data);
        jupiter_pages_num[i].data = RT_NULL;
    }

    rt_free_large((void *)olpc_data->fb);
    olpc_data->fb = RT_NULL;
}

/**
 * olpc jupiter refresh page
 */
static rt_err_t olpc_jupiter_region_refresh(struct olpc_jupiter_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int32_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = JUPITER_RGB332_WIN;
    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    wincfg->lut     = bpp_lut;
    wincfg->lutsize = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
    wincfg->fb    = olpc_data->fb;
    wincfg->w     = JUPITER_FB_W;
    wincfg->h     = JUPITER_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h;
    wincfg->x     = JUPITER_REGION_X + (JUPITER_REGION_W - JUPITER_FB_W) / 2;
    wincfg->y     = JUPITER_REGION_Y + (JUPITER_REGION_H - JUPITER_FB_H) / 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    xoffset = 0;
    yoffset = 0;

    //rt_kprintf("pic_id = %d\n", olpc_data->pic_id);
    img_info = &jupiter_pages_num[olpc_data->pic_id];
    RT_ASSERT(img_info->w <= wincfg->w);
    RT_ASSERT(img_info->h <= wincfg->h);
    rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);

    return RT_EOK;
}

/**
 * olpc jupiter refresh page
 */
static rt_err_t olpc_jupiter_time_refresh(struct olpc_jupiter_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = &jupiter_time_info;;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = JUPITER_TIME_RGB332_WIN;
    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    wincfg->lut     = bpp_lut;
    wincfg->lutsize = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
    wincfg->fb    = (rt_uint8_t *)img_info->data;
    wincfg->w     = img_info->w;
    wincfg->h     = img_info->h;
    wincfg->fblen = wincfg->w * wincfg->h;
    wincfg->x     = JUPITER_REGION_X + (JUPITER_REGION_W - wincfg->w) / 2;
    wincfg->y     = JUPITER_REGION_Y + (JUPITER_REGION_H - wincfg->h) / 2 - 60;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;
    wincfg->colorkey = COLOR_KEY_EN | 0;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);

    return RT_EOK;
}

/**
 * olpc jupiter refresh process
 */
static rt_err_t olpc_jupiter_disp_finish(void)
{
    rt_err_t ret;

    ret = rt_event_send(g_olpc_data->disp_event, EVENT_JUPITER_DISP);

    return ret;
}

static rt_err_t olpc_jupiter_disp_wait(void)
{
    rt_err_t ret;
    uint32_t event;

    ret = rt_event_recv(g_olpc_data->disp_event, EVENT_JUPITER_DISP,
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                        RT_WAITING_FOREVER, &event);
    RT_ASSERT(ret == RT_EOK);

    return RT_EOK;
}

static rt_err_t olpc_jupiter_task_fun(struct olpc_jupiter_data *olpc_data)
{
    rt_err_t ret;
    struct rt_display_mq_t disp_mq;

    rt_memset(&disp_mq, 0, sizeof(struct rt_display_mq_t));
    disp_mq.win[0].zpos = WIN_BOTTOM_LAYER;
    disp_mq.win[1].zpos = WIN_TOP_LAYER;

    //rt_tick_t ticks = rt_tick_get();

    if ((olpc_data->cmd & UPDATE_JUPITER) == UPDATE_JUPITER)
    {
        olpc_data->cmd &= ~UPDATE_JUPITER;

        olpc_jupiter_region_refresh(olpc_data, &disp_mq.win[0]);
        disp_mq.cfgsta |= (0x01 << 0);

        olpc_jupiter_time_refresh(olpc_data, &disp_mq.win[1]);
        disp_mq.cfgsta |= (0x01 << 1);
    }

    if (disp_mq.cfgsta)
    {
        disp_mq.disp_finish = olpc_jupiter_disp_finish;
        ret = rt_mq_send(olpc_data->disp->disp_mq, &disp_mq, sizeof(struct rt_display_mq_t));
        RT_ASSERT(ret == RT_EOK);
        olpc_jupiter_disp_wait();
    }

    //rt_kprintf("ticks = %d\n", rt_tick_get() - ticks);

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * olpc jupiter touch functions
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)
/**
 * screen touch.
 */
static image_info_t screen_item;
static rt_err_t olpc_jupiter_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_jupiter_data *olpc_data = (struct olpc_jupiter_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_LONG_DOWN:
        rt_event_send(olpc_data->disp_event, EVENT_JUPITER_EXIT);
        return RT_EOK;

    default:
        break;
    }

    return ret;
}

static rt_err_t olpc_jupiter_screen_touch_register(void *parameter)
{
    image_info_t *img_info = &screen_item;
    struct olpc_jupiter_data *olpc_data = (struct olpc_jupiter_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button touch register */
    {
        screen_item.w = WIN_LAYERS_W;
        screen_item.h = WIN_LAYERS_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_jupiter_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_jupiter_screen_touch_unregister(void *parameter)
{
    image_info_t *img_info = &screen_item;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

#endif

/*
 **************************************************************************************************
 *
 * olpc jupiter demo init & thread
 *
 **************************************************************************************************
 */

/**
 * screen protection timer callback.
 */
static void olpc_jupiter_timer_callback(void *parameter)
{
    struct olpc_jupiter_data *olpc_data = (struct olpc_jupiter_data *)parameter;

    olpc_data->pic_id += JUPITER_PIC_INC_STEP;
    if (olpc_data->pic_id >= JUPITER_PIC_MAX_NUM)
    {
        olpc_data->pic_id = 0;
    }

    olpc_data->cmd |= UPDATE_JUPITER;
    rt_event_send(olpc_data->disp_event, EVENT_JUPITER_REFRESH);
}

/**
 * olpc jupiter dmeo thread.
 */
static void olpc_jupiter_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_jupiter_data *olpc_data;

    g_olpc_data = olpc_data = (struct olpc_jupiter_data *)rt_malloc(sizeof(struct olpc_jupiter_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_jupiter_data));

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_jupiter_lutset(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_jupiter_screen_touch_register(olpc_data);
#endif

    olpc_data->disp_event = rt_event_create("display_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->disp_event != RT_NULL);

    ret = olpc_jupiter_init(olpc_data);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->timer = rt_timer_create("jupitertmr",
                                       olpc_jupiter_timer_callback,
                                       (void *)olpc_data,
                                       JUPITER_SRCSAVER_TIME,
                                       RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->timer != RT_NULL);
    rt_timer_start(olpc_data->timer);

    olpc_data->pic_id = 0;
    olpc_data->cmd = UPDATE_JUPITER;
    rt_event_send(olpc_data->disp_event, EVENT_JUPITER_REFRESH);

    while (1)
    {
        ret = rt_event_recv(olpc_data->disp_event,
                            EVENT_JUPITER_REFRESH | EVENT_JUPITER_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_JUPITER_REFRESH)
        {
            ret = olpc_jupiter_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_JUPITER_EXIT)
        {
            break;
        }
    }

    /* Thread deinit */
    rt_timer_stop(olpc_data->timer);
    ret = rt_timer_delete(olpc_data->timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->timer = RT_NULL;

#if defined(RT_USING_PISCES_TOUCH)
    olpc_jupiter_screen_touch_unregister(olpc_data);
    olpc_touch_list_clear();
#endif

    olpc_jupiter_deinit(olpc_data);

    rt_event_delete(olpc_data->disp_event);
    olpc_data->disp_event = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;

    rt_event_send(olpc_main_event, EVENT_APP_CLOCK);
}

/**
 * olpc jupiter demo application init.
 */
#if defined(OLPC_DLMODULE_ENABLE)
SECTION(".param") rt_uint16_t dlmodule_thread_priority = 5;
SECTION(".param") rt_uint32_t dlmodule_thread_stacksize = 2048;
int main(int argc, char *argv[])
{
    olpc_jupiter_thread(RT_NULL);
    return RT_EOK;
}

#else
int olpc_jupiter_app_init(void)
{
    rt_thread_t rtt_jupiter;

    rtt_jupiter = rt_thread_create("olpcjupiter", olpc_jupiter_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(rtt_jupiter != RT_NULL);
    rt_thread_startup(rtt_jupiter);

    return RT_EOK;
}
//INIT_APP_EXPORT(olpc_jupiter_app_init);
#endif

#endif
