/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_EBOOK_ENABLE)

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
    //0x000000, 0x005E5E5E
    0x000000, 0x00FFFFFF
};

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */
#define EBOOK_TEXT_GRAY1_WIN   0

#define EBOOK_WIN_FB_MAX_W      ((WIN_LAYERS_W / 32) * 32)    /* align 32 */
#define EBOOK_WIN_FB_MAX_H      WIN_LAYERS_H

#define EBOOK_REGION_X          0
#define EBOOK_REGION_Y          0
#define EBOOK_REGION_W          WIN_LAYERS_W
#define EBOOK_REGION_H          WIN_LAYERS_H
#define EBOOK_FB_W              EBOOK_WIN_FB_MAX_W            /* ebook frame buffer w */
#define EBOOK_FB_H              EBOOK_WIN_FB_MAX_H            /* ebook frame buffer h */

/* Event define */
#define EVENT_EBOOK_REFRESH     (0x01UL << 0)
#define EVENT_EBOOK_EXIT        (0x01UL << 1)
#define EVENT_SRCSAVER_ENTER    (0x01UL << 2)
#define EVENT_SRCSAVER_EXIT     (0x01UL << 3)

/* Command define */
#define UPDATE_PAGE             (0x01UL << 0)

#define EBOOK_SRCSAVER_TIME     (RT_TICK_PER_SECOND * 15)

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */
extern image_info_t ebook_page1_info;
extern image_info_t ebook_page2_info;
extern image_info_t ebook_page3_info;
extern image_info_t ebook_page4_info;
extern image_info_t ebook_page5_info;

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_ebook_data
{
    rt_display_data_t disp;
    rt_timer_t    srctimer;

    rt_uint8_t *fb;
    rt_uint32_t fblen;

    rt_event_t  disp_event;
    rt_uint32_t cmd;

    rt_uint16_t page_num;
};

#define EBOOK_PAGE_MAX_NUM  5
static image_info_t *ebook_pages_num[EBOOK_PAGE_MAX_NUM] =
{
    &ebook_page1_info,
    &ebook_page2_info,
    &ebook_page3_info,
    &ebook_page4_info,
    &ebook_page5_info,
};

/*
 **************************************************************************************************
 *
 * 1bpp ebook test demo
 *
 **************************************************************************************************
 */

/**
 * olpc ebook lut set.
 */
static rt_err_t olpc_ebook_lutset(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut0;

    lut0.winId = EBOOK_TEXT_GRAY1_WIN;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    lut0.lut  = bpp1_lut;
    lut0.size = sizeof(bpp1_lut) / sizeof(bpp1_lut[0]);

    ret = rt_display_lutset(&lut0, RT_NULL, RT_NULL);
    RT_ASSERT(ret == RT_EOK);

    // clear screen
    {
        struct olpc_ebook_data *olpc_data = (struct olpc_ebook_data *)parameter;
        rt_device_t device = olpc_data->disp->device;
        struct rt_device_graphic_info info;

        ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
        RT_ASSERT(ret == RT_EOK);

        rt_display_win_clear(EBOOK_TEXT_GRAY1_WIN, RTGRAPHIC_PIXEL_FORMAT_GRAY1, 0, WIN_LAYERS_H, 0);
    }

    return ret;
}

/**
 * olpc ebook demo init.
 */
static rt_err_t olpc_ebook_init(struct olpc_ebook_data *olpc_data)
{
    rt_err_t    ret;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    olpc_data->page_num = 0;
    olpc_data->cmd = UPDATE_PAGE;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->fblen = EBOOK_WIN_FB_MAX_W * EBOOK_WIN_FB_MAX_H / 8;
    olpc_data->fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->fblen);
    RT_ASSERT(olpc_data->fb != RT_NULL);

    return RT_EOK;
}

/**
 * olpc ebook demo deinit.
 */
static void olpc_ebook_deinit(struct olpc_ebook_data *olpc_data)
{
    rt_free_large((void *)olpc_data->fb);
    olpc_data->fb = RT_NULL;
}

/**
 * olpc ebook refresh page
 */
static rt_err_t olpc_ebook_page_region_refresh(struct olpc_ebook_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int32_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = EBOOK_TEXT_GRAY1_WIN;
    wincfg->fb    = olpc_data->fb;
    wincfg->w     = ((EBOOK_FB_W + 31) / 32) * 32;
    wincfg->h     = EBOOK_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = EBOOK_REGION_X + (EBOOK_REGION_W - EBOOK_FB_W) / 2;
    wincfg->y     = EBOOK_REGION_Y + (EBOOK_REGION_H - EBOOK_FB_H) / 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    /* Page update */
    {
        xoffset = 0;
        yoffset = 0;
        //draw pages
        img_info = ebook_pages_num[olpc_data->page_num];
        RT_ASSERT(img_info->w <= wincfg->w);
        RT_ASSERT(img_info->h <= wincfg->h);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
    }

    return RT_EOK;
}

/**
 * olpc ebook refresh process
 */
static rt_err_t olpc_ebook_task_fun(struct olpc_ebook_data *olpc_data)
{
    rt_err_t ret;
    struct rt_display_config  wincfg;
    struct rt_display_config *winhead = RT_NULL;

    rt_memset(&wincfg, 0, sizeof(struct rt_display_config));

    if ((olpc_data->cmd & UPDATE_PAGE) == UPDATE_PAGE)
    {
        olpc_data->cmd &= ~UPDATE_PAGE;
        olpc_ebook_page_region_refresh(olpc_data, &wincfg);
    }

    //refresh screen
    rt_display_win_layers_list(&winhead, &wincfg);
    ret = rt_display_win_layers_set(winhead);
    RT_ASSERT(ret == RT_EOK);

    if (olpc_data->cmd != 0)
    {
        rt_event_send(olpc_data->disp_event, EVENT_EBOOK_REFRESH);
    }

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * olpc ebook touch functions
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)
/**
 * screen touch.
 */
#define BL_MOVE_STEP_MIN    (100)
#define PAGE_MOVE_STEP_MIN  (440)
static image_info_t screen_item;
static struct point_info down_point;
static struct point_info up_point;
static uint8_t page_bl_flag = 0;
static rt_err_t olpc_ebook_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_ebook_data *olpc_data = (struct olpc_ebook_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_LONG_DOWN:
        rt_kprintf("ebook long down!!\n");
        rt_event_send(olpc_data->disp_event, EVENT_EBOOK_EXIT);
        return RT_EOK;

    case TOUCH_EVENT_SHORT_DOWN:
        page_bl_flag = 0;
        down_point.x = point->x;
        down_point.y = point->y;
        ret = RT_EOK;
        break;

    case TOUCH_EVENT_MOVE:
        //check page move or bl move
        if (page_bl_flag == 0)
        {
            rt_int16_t xdiff = point->x - down_point.x;
            rt_int16_t ydiff = point->y - down_point.y;

            if (ABS(xdiff) >= PAGE_MOVE_STEP_MIN)
            {
                //rt_kprintf("page move!!!!\n");
                page_bl_flag = 1;
            }
            else if ((ABS(ydiff) >= BL_MOVE_STEP_MIN) && (ABS(xdiff) < BL_MOVE_STEP_MIN))
            {
                //rt_kprintf("bl move!!!!\n");
                page_bl_flag = 2;
            }
        }

        // page move
        if (page_bl_flag == 1)
        {
            up_point.x = point->x;
            up_point.y = point->y;
        }
        // backlight move
        else if (page_bl_flag == 2)
        {
            rt_int32_t xdiff = point->x - down_point.x;
            rt_int32_t ydiff = -(point->y - down_point.y);

            // if x out of range reset touch
            if (ABS(xdiff) > BL_MOVE_STEP_MIN)
            {
                rt_kprintf("touch reset\n");
                olpc_touch_reset();
            }

            if (ABS(ydiff) > BL_MOVE_STEP_MIN)
            {
                down_point.y = point->y;

                struct rt_device_graphic_info info;
                rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

                olpc_data->disp->blval += ydiff * rt_display_get_bl_max(olpc_data->disp->device) / WIN_LAYERS_H * 2;
                if ((rt_int16_t)olpc_data->disp->blval < 0)
                {
                    olpc_data->disp->blval = 0;
                }
                else if ((rt_int16_t)olpc_data->disp->blval > rt_display_get_bl_max(olpc_data->disp->device))
                {
                    olpc_data->disp->blval = rt_display_get_bl_max(olpc_data->disp->device);
                }

                rt_display_win_backlight_set(olpc_data->disp->blval);
            }

            ret = RT_EOK;
        }
        break;

    case TOUCH_EVENT_UP:
    {
        if (page_bl_flag == 1)
        {
            rt_int16_t diff = up_point.x - down_point.x;
            if (diff < -PAGE_MOVE_STEP_MIN)
            {
                if (olpc_data->page_num < EBOOK_PAGE_MAX_NUM - 1)
                {
                    olpc_data->page_num ++;
                }
                olpc_data->cmd |= UPDATE_PAGE;
                rt_event_send(olpc_data->disp_event, EVENT_EBOOK_REFRESH);
                ret = RT_EOK;
            }
            else if (diff > PAGE_MOVE_STEP_MIN)
            {
                if (olpc_data->page_num > 0)
                {
                    olpc_data->page_num --;
                }
                olpc_data->cmd |= UPDATE_PAGE;
                rt_event_send(olpc_data->disp_event, EVENT_EBOOK_REFRESH);
                ret = RT_EOK;
            }
        }
    }
    break;

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

static rt_err_t olpc_ebook_screen_touch_register(void *parameter)
{
    image_info_t *img_info = &screen_item;
    struct olpc_ebook_data *olpc_data = (struct olpc_ebook_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button touch register */
    {
        screen_item.w = WIN_LAYERS_W;
        screen_item.h = WIN_LAYERS_H;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_ebook_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_ebook_screen_touch_unregister(void *parameter)
{
    image_info_t *img_info = &screen_item;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

#endif

/*
 **************************************************************************************************
 *
 * olpc ebook screen protection functions
 *
 **************************************************************************************************
 */
#if defined(OLPC_APP_SRCSAVER_ENABLE)

/**
 * ebook screen protection timeout callback.
 */
static void olpc_ebook_srcprotect_callback(void *parameter)
{
    struct olpc_ebook_data *olpc_data = (struct olpc_ebook_data *)parameter;
    rt_event_send(olpc_data->disp_event, EVENT_SRCSAVER_ENTER);
}

/**
 * exit screen protection hook.
 */
static rt_err_t olpc_ebook_screen_protection_hook(void *parameter)
{
    struct olpc_ebook_data *olpc_data = (struct olpc_ebook_data *)parameter;

    rt_event_send(olpc_data->disp_event, EVENT_SRCSAVER_EXIT);

    return RT_EOK;
}

/**
 * start screen protection.
 */
static rt_err_t olpc_ebook_screen_protection_enter(void *parameter)
{
    struct olpc_ebook_data *olpc_data = (struct olpc_ebook_data *)parameter;

    rt_timer_stop(olpc_data->srctimer);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_ebook_screen_touch_unregister(parameter);
    olpc_touch_list_clear();
#endif

    // register exit hook & start screen protection
    olpc_srcprotect_app_init(olpc_ebook_screen_protection_hook, parameter);

    return RT_EOK;
}

/**
 * exit screen protection.
 */
static rt_err_t olpc_ebook_screen_protection_exit(void *parameter)
{
    struct olpc_ebook_data *olpc_data = (struct olpc_ebook_data *)parameter;

    olpc_ebook_lutset(olpc_data);   // reset lut

#if defined(RT_USING_PISCES_TOUCH)
    olpc_ebook_screen_touch_register(parameter);
#endif

    olpc_data->cmd |= UPDATE_PAGE;
    rt_event_send(olpc_data->disp_event, EVENT_EBOOK_REFRESH);

    rt_timer_start(olpc_data->srctimer);

    return RT_EOK;
}
#endif

/*
 **************************************************************************************************
 *
 * olpc ebook demo init & thread
 *
 **************************************************************************************************
 */

/**
 * olpc ebook dmeo thread.
 */
static void olpc_ebook_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_ebook_data *olpc_data;

    olpc_data = (struct olpc_ebook_data *)rt_malloc(sizeof(struct olpc_ebook_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_ebook_data));

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_ebook_lutset(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_ebook_screen_touch_register(olpc_data);
#endif

    olpc_data->disp_event = rt_event_create("display_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->disp_event != RT_NULL);

    ret = olpc_ebook_init(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_SRCSAVER_ENABLE)
    olpc_data->srctimer = rt_timer_create("ebookprotect",
                                          olpc_ebook_srcprotect_callback,
                                          (void *)olpc_data,
                                          EBOOK_SRCSAVER_TIME,
                                          RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->srctimer != RT_NULL);

    rt_timer_start(olpc_data->srctimer);
#endif

    rt_event_send(olpc_data->disp_event, EVENT_EBOOK_REFRESH);

    while (1)
    {
        ret = rt_event_recv(olpc_data->disp_event,
                            EVENT_EBOOK_REFRESH | EVENT_EBOOK_EXIT | EVENT_SRCSAVER_ENTER | EVENT_SRCSAVER_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_EBOOK_REFRESH)
        {
            ret = olpc_ebook_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

#if defined(OLPC_APP_SRCSAVER_ENABLE)
        if (event & EVENT_SRCSAVER_ENTER)
        {
            ret = olpc_ebook_screen_protection_enter(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_SRCSAVER_EXIT)
        {
            ret = olpc_ebook_screen_protection_exit(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }
#endif
        if (event & EVENT_EBOOK_EXIT)
        {
            break;
        }
    }

    /* Thread deinit */
#if defined(OLPC_APP_SRCSAVER_ENABLE)
    rt_timer_stop(olpc_data->srctimer);
    ret = rt_timer_delete(olpc_data->srctimer);
    olpc_data->srctimer = RT_NULL;
    RT_ASSERT(ret == RT_EOK);
#endif

#if defined(RT_USING_PISCES_TOUCH)
    olpc_ebook_screen_touch_unregister(olpc_data);
    olpc_touch_list_clear();
#endif

    olpc_ebook_deinit(olpc_data);

    rt_event_delete(olpc_data->disp_event);
    olpc_data->disp_event = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;

    rt_event_send(olpc_main_event, EVENT_APP_CLOCK);
}

/**
 * olpc ebook demo application init.
 */
#if defined(OLPC_DLMODULE_ENABLE)
SECTION(".param") rt_uint16_t dlmodule_thread_priority = 5;
SECTION(".param") rt_uint32_t dlmodule_thread_stacksize = 2048;
int main(int argc, char *argv[])
{
    olpc_ebook_thread(RT_NULL);
    return RT_EOK;
}

#else
int olpc_ebook_app_init(void)
{
    rt_thread_t rtt_ebook;

    rtt_ebook = rt_thread_create("olpcebook", olpc_ebook_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(rtt_ebook != RT_NULL);
    rt_thread_startup(rtt_ebook);

    return RT_EOK;
}
//INIT_APP_EXPORT(olpc_ebook_app_init);
#endif

#endif
