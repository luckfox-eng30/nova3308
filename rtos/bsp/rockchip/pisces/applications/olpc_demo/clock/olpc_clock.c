/**
  * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#if defined(OLPC_APP_CLOCK_ENABLE)

#include "drv_heap.h"
#include "applications/common/image_info.h"
#include "applications/common/olpc_display.h"

#if defined(RT_USING_PISCES_TOUCH)
#include "drv_touch.h"
#include "applications/common/olpc_touch.h"
#endif

#if defined(RT_USING_DTCM_HEAP) && defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
#define OLPC_CLOCKBKG_USE_DTCM
#endif

/**
 * color palette for 1bpp
 */
static uint32_t bpp1_lut[2] =
{
    0x00000000, 0x00ffffff
};

/*
 **************************************************************************************************
 *
 * Macro define
 *
 **************************************************************************************************
 */
/* display win layers */
#define CLOCK_GRAY1_WIN     0
#define CLOCK_RGB332_WIN    0
#define CLOCK_RGB565_WIN    0
#define CLOCKBKG_RGB565_WIN 2
#define BLN_RGB332_WIN      1

/* display region define */
#define LAMP_MAX_NUM         8
#define LAMP_HORIZONTAL_NUM  17
#define LAMP_VERTICAL_NUM    34

#define LAMP_HORIZONTAL_SIZE 34
#define LAMP_VERTICAL_SIZE   34
#define LAMP_HORIZONTAL_STEP 64
#define LAMP_VERTICAL_STEP   64
#define LAMP_HORIZONTAL_DIFF (LAMP_HORIZONTAL_STEP - LAMP_HORIZONTAL_SIZE)                                  // 30:   30   = 64 - 34
#define LAMP_VERTICAL_DIFF   (LAMP_VERTICAL_STEP   - LAMP_VERTICAL_SIZE  )                                  // 30:   30   = 64 - 34

#define LAMP_REGION_W       (((LAMP_HORIZONTAL_NUM  ) * LAMP_HORIZONTAL_STEP) - LAMP_HORIZONTAL_DIFF)       // 1058: 1058 = 64 *  17    - 30
#define LAMP_REGION_H       (((LAMP_VERTICAL_NUM + 2) * LAMP_VERTICAL_STEP  ) - LAMP_VERTICAL_DIFF  )       // 2274: 2274 = 64 * (34+2) - 30

#define LAMP_REGION_X       ((WIN_LAYERS_W - LAMP_REGION_W) / 2)                                            // 11:   1080 = 11 + 1058 + 11
#define LAMP_REGION_Y       (((((WIN_LAYERS_H - LAMP_REGION_H) / 2) + 1) / 2) * 2)                          // 34:   2340 = 34 + 2274 + 32
#define LAMP_FB_W           (((LAMP_MAX_NUM * LAMP_HORIZONTAL_STEP - LAMP_HORIZONTAL_DIFF + 3) / 4) * 4)    // 484:  484
#define LAMP_FB_H           (((LAMP_VERTICAL_SIZE + 3) / 4) * 4)                                            // 36:   36

#define BLN_REGION_X        20
#define BLN_REGION_Y        20
#define BLN_REGION_W        120
#define BLN_REGION_H        120
#define BLN_FB_W            120
#define BLN_FB_H            120

#define CLOCK_REGION_X      (LAMP_REGION_X + ((LAMP_HORIZONTAL_SIZE + 3) / 4) * 4)                          // 47:   47   = 11 + 36
#define CLOCK_REGION_Y      (LAMP_REGION_Y + ((LAMP_VERTICAL_SIZE   + 3) / 4) * 4 + 80)  // 150             // 70:   70   = 34 + 36
#define CLOCK_REGION_W      (WIN_LAYERS_W - CLOCK_REGION_X * 2)                                             // 986:  1080 = 11 + 36 + 986 + 36 + 11
#define CLOCK_REGION_H      (640 - 80)
#define CLOCK_FB_W          440
#define CLOCK_FB_H          440
#define CLOCK_MOVE_XSTEP    200
#define CLOCK_MOVE_YSTEP    100

#define MSG_REGION_X        CLOCK_REGION_X
#define MSG_REGION_Y        (CLOCK_REGION_Y + CLOCK_REGION_H + LAMP_VERTICAL_STEP * 1)                      // 774:  774  = 70 + 640 + 64 * 1
#define MSG_REGION_W        CLOCK_REGION_W
#define MSG_REGION_H        400
#define MSG_FB_W            440
#define MSG_FB_H            300
#define MSG_MOVE_XSTEP      200
#define MSG_MOVE_YSTEP      50

#define HOME_REGION_X       CLOCK_REGION_X
#define HOME_REGION_Y       (MSG_REGION_Y + MSG_REGION_H + LAMP_VERTICAL_STEP * 1)                          // 1238: 1238 = 774 + 400 + 64 * 1
#define HOME_REGION_W       CLOCK_REGION_W
#define HOME_REGION_H       540
#define HOME_FB_W           544     /* home frame buffer w */
#define HOME_FB_H           540     /* home frame buffer h */

#define LOCK_REGION_X       CLOCK_REGION_X
#define LOCK_REGION_Y       (MSG_REGION_Y + MSG_REGION_H + LAMP_VERTICAL_STEP * 12)                         // 1942: 1942 = 774 + 400 + 64 * 12
#define LOCK_REGION_W       CLOCK_REGION_W
#define LOCK_REGION_H       162
#define LOCK_FB_W           800
#define LOCK_FB_H           162

#define FP_REGION_X         CLOCK_REGION_X
#define FP_REGION_Y         (HOME_REGION_Y + HOME_REGION_H + LAMP_VERTICAL_STEP * 3)                        // 1906: 1906 = 1238 + 540 + 64 * 2
#define FP_REGION_W         CLOCK_REGION_W
#define FP_REGION_H         150
#define FP_FB_W             152
#define FP_FB_H             150


/* Event define for 'disp_event' */
#define EVENT_CLOCK_UPDATE  (0x01UL << 0)
#define EVENT_CLOCK_EXIT    (0x01UL << 1)
#define EVENT_CLOCK_DISP    (0x01UL << 2)

#define EVENT_BLN_UPDATE    (0x01UL << 3)
#define EVENT_BLN_DISP      (0x01UL << 4)
#define EVENT_BLN_EXIT      (0x01UL << 5)
#define EVENT_BLN_EXIT_DONE (0x01UL << 6)

/* Command define for 'blncmd' */
#define UPDATE_BLN          (0x01UL << 0)

/* Command define for 'cmd' */
#define UPDATE_CLOCK        (0x01UL << 0)
#define UPDATE_MSG          (0x01UL << 1)
#define UPDATE_HOME         (0x01UL << 2)
#define UPDATE_FINGERP      (0x01UL << 3)
#define UPDATE_LOCK         (0x01UL << 4)
#define MOVE_CLOCK          (0x01UL << 5)
#define MOVE_MSG            (0x01UL << 6)

/* Screen state define */
#define SCREEN_OFF          0
#define SCREEN_LOCK         1
#define SCREEN_HOME         2

/* Region ID define */
#define REGION_ID_NULL      0
#define REGION_ID_CLOCK     1
#define REGION_ID_MSG       2
#define REGION_ID_HOME      3
#define REGION_ID_FP        4
#define REGION_ID_LOCK      5

#define LAMP_POS_TOP        0
#define LAMP_POS_RIGHT      1
#define LAMP_POS_BOTTOM     2
#define LAMP_POS_LEFT       3

#define CLOCK_UPDATE_TIME   (RT_TICK_PER_SECOND)
#define SCREEN_HOLD_TIME    (RT_TICK_PER_SECOND * 5)
#define FP_LOOP_TIME        (RT_TICK_PER_SECOND / 10)
#define LAMP_LOOP_TIME      (RT_TICK_PER_SECOND / 5)

#define BLN_ROTATE_TICKS        ((RT_TICK_PER_SECOND / 1000) * 33)
#define OLPC_ROTATE_PARAM_STEP  1
#define BLN_ROTATE_TIME         900
#define BLN_BREADTH_TIME        3000

/*
 **************************************************************************************************
 *
 * Global static struct & data define
 *
 **************************************************************************************************
 */
struct olpc_clock_data
{
    rt_display_data_t disp;
    rt_event_t        disp_event;

#if 1 //defined(OLPC_APP_CLOCK_CLOCK_ENABLE)
    rt_uint32_t cmd;

    rt_uint8_t *fb;
    rt_uint32_t fblen;
#if defined(OLPC_CLOCKBKG_USE_DTCM)
    rt_uint8_t *bkgfb;
    rt_uint32_t bkgfblen;
#endif
    rt_uint8_t  screen_sta;

    rt_timer_t  clock_timer;
    rt_timer_t  src_timer;
    rt_timer_t  fp_timer;

    rt_uint8_t  hour;
    rt_uint8_t  minute;
    rt_uint8_t  second;
    rt_uint32_t ticks;
    rt_uint8_t  month;
    rt_uint8_t  day;
    rt_uint8_t  week;

    rt_uint8_t  batval;
    rt_uint32_t moval;
    rt_uint8_t  musicval;
    rt_uint8_t  qqval;
    rt_uint8_t  webval;
    rt_uint8_t  wexval;

    rt_int16_t  clock_xoffset;
    rt_int16_t  clock_yoffset;
    rt_int8_t   clock_xdir;
    rt_int8_t   clock_ydir;
    rt_int16_t  clock_ylast;

    rt_int16_t  msg_xoffset;
    rt_int16_t  msg_yoffset;
    rt_int16_t  msg_xdir;
    rt_int16_t  msg_ydir;
    rt_int16_t  msg_ylast;

    rt_uint8_t  fp_id;
    rt_uint8_t  home_id;

    rt_int16_t  lock_px;
    rt_int16_t  lock_py;
#endif

#if defined(OLPC_APP_CLOCK_BLN_ENABLE)
    rt_uint32_t blncmd;
    rt_uint8_t *blnfb;
    rt_uint32_t blnfblen;
    rt_timer_t  bln_timer;
    rt_uint16_t rotatenum;
    rt_uint16_t rotatemax;
#endif
};

static struct olpc_clock_data *g_olpc_data = RT_NULL;

/*
 **************************************************************************************************
 *
 * Declaration
 *
 **************************************************************************************************
 */

#if defined(RT_USING_PISCES_TOUCH)
static rt_err_t olpc_clock_screen_touch_register(void *parameter);
static rt_err_t olpc_clock_screen_touch_unregister(void *parameter);
static rt_err_t olpc_clock_home_touch_register(void *parameter);
static rt_err_t olpc_clock_home_touch_unregister(void *parameter);
static rt_err_t olpc_clock_fingerprint_touch_register(void *parameter);
static rt_err_t olpc_clock_fingerprint_touch_unregister(void *parameter);
static rt_err_t olpc_clock_lock_touch_register(void *parameter);
static rt_err_t olpc_clock_lock_touch_unregister(void *parameter);
#endif

/*
 **************************************************************************************************
 *
 * breathlight functions
 *
 **************************************************************************************************
 */
#if defined(OLPC_APP_CLOCK_BLN_ENABLE)

extern image_info_t clock_bln03_info;
extern image_info_t clock_bln05_info;
extern image_info_t clock_bln07_info;
extern image_info_t clock_bln09_info;
extern image_info_t clock_bln11_info;
extern image_info_t clock_bln13_info;
extern image_info_t clock_bln15_info;
extern image_info_t clock_bln17_info;
extern image_info_t clock_bln19_info;
extern image_info_t clock_bln21_info;
extern image_info_t clock_bln23_info;
extern image_info_t clock_bln25_info;
extern image_info_t clock_bln27_info;
extern image_info_t clock_bln29_info;
extern image_info_t clock_bln31_info;
extern image_info_t clock_bln33_info;
extern image_info_t clock_bln35_info;
extern image_info_t clock_bln37_info;
extern image_info_t clock_bln39_info;
extern image_info_t clock_bln41_info;
extern image_info_t clock_bln43_info;
extern image_info_t clock_bln45_info;
extern image_info_t clock_bln47_info;
extern image_info_t clock_bln49_info;
extern image_info_t clock_bln51_info;
extern image_info_t clock_bln53_info;

#define BLN_ITEM_MAX    26
static const image_info_t *bln_item[BLN_ITEM_MAX] =
{
    &clock_bln03_info,
    &clock_bln05_info,
    &clock_bln07_info,
    &clock_bln09_info,
    &clock_bln11_info,
    &clock_bln13_info,
    &clock_bln15_info,
    &clock_bln17_info,
    &clock_bln19_info,
    &clock_bln21_info,
    &clock_bln23_info,
    &clock_bln25_info,
    &clock_bln27_info,
    &clock_bln29_info,
    &clock_bln31_info,
    &clock_bln33_info,
    &clock_bln35_info,
    &clock_bln37_info,
    &clock_bln39_info,
    &clock_bln41_info,
    &clock_bln43_info,
    &clock_bln45_info,
    &clock_bln47_info,
    &clock_bln49_info,
    &clock_bln51_info,
    &clock_bln53_info,
};

/**
 * breathlight refresh timer callback.
 */
static void olpc_bln_timeout(void *parameter)
{
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;
    rt_uint32_t ticks;

    //if (olpc_data->rotatenum * BLN_ROTATE_TICKS < BLN_ROTATE_TIME)
    if (olpc_data->rotatenum < BLN_ITEM_MAX)
    {
        olpc_data->blncmd |= UPDATE_BLN;
        rt_event_send(olpc_data->disp_event, EVENT_BLN_UPDATE);

        olpc_data->rotatenum += OLPC_ROTATE_PARAM_STEP;
        if (olpc_data->rotatenum < BLN_ITEM_MAX)
        {
            ticks = BLN_ROTATE_TICKS;
        }
        else
        {
            olpc_data->rotatenum = 0;
            ticks = BLN_BREADTH_TIME - BLN_ROTATE_TIME;
        }
    }
    else
    {
        olpc_data->rotatenum = 0;
        olpc_data->blncmd |= UPDATE_BLN;
        rt_event_send(olpc_data->disp_event, EVENT_BLN_UPDATE);
        olpc_data->rotatenum += OLPC_ROTATE_PARAM_STEP;
    }

    rt_timer_control(olpc_data->bln_timer, RT_TIMER_CTRL_SET_TIME, &ticks);
    rt_timer_start(olpc_data->bln_timer);
}

/**
 * breathlight region refresh.
 */
static rt_err_t olpc_clock_bln_region_refresh(struct olpc_clock_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = BLN_RGB332_WIN;
    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    wincfg->lut     = bpp_lut;
    wincfg->lutsize = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
    wincfg->fb    = olpc_data->blnfb;
    wincfg->w     = ((BLN_FB_W + 3) / 4) * 4;
    wincfg->h     = BLN_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h;
    wincfg->x     = BLN_REGION_X + (BLN_REGION_W - BLN_FB_W) / 2;
    wincfg->y     = BLN_REGION_Y + (BLN_REGION_H - BLN_FB_H) / 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->blnfblen);

    rt_memset((void *)wincfg->fb, 0x00, wincfg->fblen);

    xoffset = 0;
    yoffset = 0;

    //if (olpc_data->screen_sta == SCREEN_HOME)
    {
        // draw breathlight
        if (olpc_data->rotatenum >= BLN_ITEM_MAX)
        {
            olpc_data->rotatenum--;
        }
        img_info = (image_info_t *)bln_item[olpc_data->rotatenum];
        RT_ASSERT(img_info->w <= wincfg->w);
        RT_ASSERT(img_info->h <= wincfg->h);

        yoffset  += 0;
        xoffset  += (BLN_FB_W - img_info->w) / 2;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);
    }

    return RT_EOK;
}

/**
 * breathlight display finish callback.
 */
static rt_err_t olpc_bln_disp_finish(void)
{
    rt_err_t ret;

    ret = rt_event_send(g_olpc_data->disp_event, EVENT_BLN_DISP);

    return ret;
}

/**
 * breathlight display wait finish.
 */
static rt_err_t olpc_bln_disp_wait(void)
{
    rt_err_t ret;
    uint32_t event;

    ret = rt_event_recv(g_olpc_data->disp_event, EVENT_BLN_DISP,
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                        RT_WAITING_FOREVER, &event);
    RT_ASSERT(ret == RT_EOK);

    return RT_EOK;
}

/**
 * breathlight message process function.
 */
static rt_err_t olpc_bln_task_fun(struct olpc_clock_data *olpc_data)
{
    rt_err_t ret;
    struct rt_display_mq_t disp_mq;
    rt_uint32_t updatecmd = olpc_data->blncmd;

    rt_memset(&disp_mq, 0, sizeof(struct rt_display_mq_t));
    disp_mq.win[0].zpos = WIN_TOP_LAYER;

    if ((updatecmd & UPDATE_BLN) == UPDATE_BLN)
    {
        updatecmd &= ~UPDATE_BLN;

        ret = olpc_clock_bln_region_refresh(olpc_data, &disp_mq.win[0]);
        RT_ASSERT(ret == RT_EOK);
        disp_mq.cfgsta |= (0x01 << 0);
    }

    if (disp_mq.cfgsta)
    {
        disp_mq.disp_finish = olpc_bln_disp_finish;
        ret = rt_mq_send(olpc_data->disp->disp_mq, &disp_mq, sizeof(struct rt_display_mq_t));
        RT_ASSERT(ret == RT_EOK);
        olpc_bln_disp_wait();
    }

    return RT_EOK;
}

/**
 * breathlight thread.
 */
static void olpc_bln_thread(void *p)
{
    rt_err_t ret;
    uint32_t event;
    struct olpc_clock_data *olpc_data = g_olpc_data;

    olpc_data->blnfblen = BLN_FB_W * BLN_FB_H;
    olpc_data->blnfb    = (rt_uint8_t *)rt_malloc_large(olpc_data->blnfblen);
    RT_ASSERT(olpc_data->blnfb != RT_NULL);

    olpc_data->bln_timer = rt_timer_create("blntimer",
                                           olpc_bln_timeout,
                                           (void *)olpc_data,
                                           BLN_ROTATE_TICKS,
                                           RT_TIMER_FLAG_ONE_SHOT);
    RT_ASSERT(olpc_data->bln_timer != RT_NULL);
    rt_timer_start(olpc_data->bln_timer);

    while (1)
    {
        ret = rt_event_recv(olpc_data->disp_event, EVENT_BLN_UPDATE | EVENT_BLN_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_BLN_UPDATE)
        {
            ret = olpc_bln_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_BLN_EXIT)
        {
            break;
        }
    }

    rt_timer_stop(olpc_data->bln_timer);
    ret = rt_timer_delete(olpc_data->bln_timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->bln_timer = RT_NULL;

    rt_free_large((void *)olpc_data->blnfb);
    olpc_data->blnfb = RT_NULL;

    rt_event_send(olpc_data->disp_event, EVENT_BLN_EXIT_DONE);
}
#endif

/*
 **************************************************************************************************
 *
 * clock regions refresh
 *
 **************************************************************************************************
 */

extern image_info_t clock_bkg_info;
extern image_info_t clock_sec_info;
extern image_info_t clock_min_info;
extern image_info_t clock_hour_info;
#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN332) || defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
extern image_info_t clock_centre_info;
#endif

/**
 * Clock timer callback.
 */
static void olpc_clock_timeout(void *parameter)
{
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;

    ++olpc_data->second;
    if (olpc_data->second >= 60)
    {
        olpc_data->second = 0;
        if (++olpc_data->minute == 60)
        {
            olpc_data->minute = 0;
            if (++olpc_data->hour >= 24)
            {
                olpc_data->hour = 0;

                if (++olpc_data->week == 7)
                {
                    olpc_data->week = 0;
                }

                if (++olpc_data->day > 31)
                {
                    olpc_data->day = 1;
                    if (++olpc_data->month > 12)
                    {
                        olpc_data->month = 1;
                    }
                }
                olpc_data->cmd |= UPDATE_MSG;
            }
        }

        if ((olpc_data->minute % 3) == 0)
        {
            olpc_data->cmd |= MOVE_CLOCK | UPDATE_CLOCK;
            olpc_data->cmd |= MOVE_MSG | UPDATE_MSG;
        }
    }

    olpc_data->cmd |= UPDATE_CLOCK;
    rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);
}

/**
 * Clock region move.
 */
static rt_err_t olpc_clock_clock_region_move(struct olpc_clock_data *olpc_data)
{
    struct rt_device_graphic_info info;
    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    olpc_data->clock_xoffset += olpc_data->clock_xdir * CLOCK_MOVE_XSTEP;
    if (olpc_data->clock_xoffset > (CLOCK_REGION_W - CLOCK_FB_W) / 2)
    {
        olpc_data->clock_xoffset = (CLOCK_REGION_W - CLOCK_FB_W) / 2;
        olpc_data->clock_xdir = -1 * olpc_data->clock_xdir;
    }
    if (olpc_data->clock_xoffset <= (CLOCK_FB_W - CLOCK_REGION_W) / 2)
    {
        olpc_data->clock_xoffset = (CLOCK_FB_W - CLOCK_REGION_W) / 2;
        olpc_data->clock_xdir = -1 * olpc_data->clock_xdir;
    }

    olpc_data->clock_yoffset += olpc_data->clock_ydir * CLOCK_MOVE_YSTEP;
    if (olpc_data->clock_yoffset > (CLOCK_REGION_H - CLOCK_FB_H) / 2)
    {
        olpc_data->clock_yoffset = (CLOCK_REGION_H - CLOCK_FB_H) / 2;
        olpc_data->clock_ydir = -1 * olpc_data->clock_ydir;
    }
    if (olpc_data->clock_yoffset <= (CLOCK_FB_H - CLOCK_REGION_H) / 2)
    {
        olpc_data->clock_yoffset = (CLOCK_FB_H - CLOCK_REGION_H) / 2;
        olpc_data->clock_ydir = -1 * olpc_data->clock_ydir;
    }

    return RT_EOK;
}

#if defined(OLPC_CLOCKBKG_USE_DTCM)
/**
 * Clock region refresh.
 */
static rt_err_t olpc_clock_clock_bkg_refresh(struct olpc_clock_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_device_t  device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId  = CLOCKBKG_RGB565_WIN;
    wincfg->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    wincfg->lut    = RT_NULL;
    wincfg->lutsize = 0;
    wincfg->fb    = olpc_data->bkgfb;
    wincfg->w     = ((CLOCK_FB_W + 3) / 4) * 4;
    wincfg->h     = CLOCK_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h * 2;
    wincfg->x     = CLOCK_REGION_X + (CLOCK_REGION_W - CLOCK_FB_W) / 2 + olpc_data->clock_xoffset;
    wincfg->y     = CLOCK_REGION_Y + (CLOCK_REGION_H - CLOCK_FB_H) / 2 + olpc_data->clock_yoffset;
    wincfg->y     = (wincfg->y / 2) * 2;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->y % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->bkgfblen);

    return RT_EOK;
}
#endif

/**
 * Clock region refresh.
 */
static rt_err_t olpc_clock_clock_region_refresh(struct olpc_clock_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
    wincfg->winId  = CLOCK_RGB565_WIN;
    wincfg->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    wincfg->lut    = RT_NULL;
    wincfg->lutsize = 0;
#else
    wincfg->winId   = CLOCK_RGB332_WIN;
    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    wincfg->lut     = bpp_lut;
    wincfg->lutsize = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
#endif
    wincfg->fb    = olpc_data->fb;
    wincfg->w     = ((CLOCK_FB_W + 3) / 4) * 4;
    wincfg->h     = CLOCK_FB_H;
#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
    wincfg->fblen = wincfg->w * wincfg->h * 2;
#else
    wincfg->fblen = wincfg->w * wincfg->h;
#endif
    wincfg->x     = CLOCK_REGION_X + (CLOCK_REGION_W - CLOCK_FB_W) / 2 + olpc_data->clock_xoffset;
    wincfg->y     = CLOCK_REGION_Y + (CLOCK_REGION_H - CLOCK_FB_H) / 2 + olpc_data->clock_yoffset;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = olpc_data->clock_ylast;
    olpc_data->clock_ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->y % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    rt_memset((void *)wincfg->fb, 0x00, wincfg->fblen);

    xoffset = 0;
    yoffset = 0;

    //draw background
    img_info = &clock_bkg_info;
    RT_ASSERT(img_info->w <= wincfg->w);
    RT_ASSERT(img_info->h <= wincfg->h);

    yoffset  += (CLOCK_FB_H - img_info->h) / 2;//0;
    xoffset  += (CLOCK_FB_W - img_info->w) / 2;
#ifdef OLPC_CLOCKBKG_USE_DTCM
    wincfg->colorkey = COLOR_KEY_EN | 0;
#else
    rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);
#endif

    // draw hour,min,sec
    {
        int32_t hour, angle;
        uint32_t background_w = wincfg->w;//img_info->w;

        xoffset += (img_info->w / 2);
        yoffset += (img_info->h / 2);

        //draw hour line
        img_info = &clock_hour_info;
        RT_ASSERT(img_info->w <= wincfg->w / 2);
        RT_ASSERT(img_info->h <= wincfg->h / 2);

        hour = olpc_data->hour;
        if (hour >= 12)
        {
            hour -= 12;
        }
        angle = hour * (360 / 12) + (olpc_data->minute * 30) / 60 - 90;
        if (angle < 0)
        {
            angle += 360;
        }
#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
        rt_display_rotate_16bit((float)angle, img_info->w, img_info->h, (unsigned short *)img_info->data,
                                (unsigned short *)((uint32_t)wincfg->fb + 2 * (yoffset * wincfg->w + xoffset)),
                                background_w, 4, img_info->h / 2);
#elif defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN332)
        rt_display_rotate_8bit((float)angle, img_info->w, img_info->h, (unsigned char *)img_info->data,
                               (unsigned char *)((uint32_t)wincfg->fb + (yoffset * wincfg->w + xoffset)),
                               background_w, 4, img_info->h / 2);
#else
        rt_display_rotate_8bit((float)angle, img_info->w, img_info->h, (unsigned char *)img_info->data,
                               (unsigned char *)((uint32_t)wincfg->fb + yoffset * wincfg->w + xoffset),
                               background_w, 16, img_info->h / 2);
#endif
        //draw min line
        img_info = &clock_min_info;
        RT_ASSERT(img_info->w <= wincfg->w / 2);
        RT_ASSERT(img_info->h <= wincfg->h / 2);

        angle = olpc_data->minute * (360 / 60);
        angle -= 90;
        if (angle < 0)
        {
            angle += 360;
        }
#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
        rt_display_rotate_16bit((float)angle, img_info->w, img_info->h, (unsigned short *)img_info->data,
                                (unsigned short *)((uint32_t)wincfg->fb + 2 * (yoffset * wincfg->w + xoffset)),
                                background_w, 4, img_info->h / 2);
#elif defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN332)
        rt_display_rotate_8bit((float)angle, img_info->w, img_info->h, (unsigned char *)img_info->data,
                               (unsigned char *)((uint32_t)wincfg->fb + (yoffset * wincfg->w + xoffset)),
                               background_w, 4, img_info->h / 2);
#else
        rt_display_rotate_8bit((float)angle, img_info->w, img_info->h, (unsigned char *)img_info->data,
                               (unsigned char *)((uint32_t)wincfg->fb + yoffset * wincfg->w + xoffset),
                               background_w, 18, img_info->h / 2);
#endif
        //draw second line
        img_info = &clock_sec_info;
        RT_ASSERT(img_info->w <= wincfg->w / 2);
        RT_ASSERT(img_info->h <= wincfg->h / 2);

        angle = olpc_data->second * (360 / 60);
        angle -= 90;
        if (angle < 0)
        {
            angle += 360;
        }
#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
        rt_display_rotate_16bit((float)angle, img_info->w, img_info->h, (unsigned short *)img_info->data,
                                (unsigned short *)((uint32_t)wincfg->fb + 2 * (yoffset * wincfg->w + xoffset)),
                                background_w, 4, img_info->h / 2);
#elif defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN332)
        rt_display_rotate_8bit((float)angle, img_info->w, img_info->h, (unsigned char *)img_info->data,
                               (unsigned char *)((uint32_t)wincfg->fb + (yoffset * wincfg->w + xoffset)),
                               background_w, 4, img_info->h / 2);
#else
        rt_display_rotate_8bit((float)angle, img_info->w, img_info->h, (unsigned char *)img_info->data,
                               (unsigned char *)((uint32_t)wincfg->fb + yoffset * wincfg->w + xoffset),
                               background_w, 20, img_info->h / 2);
#endif

#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN332) || defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
        //draw centre
        img_info = &clock_centre_info;
        RT_ASSERT(img_info->w <= wincfg->w);
        RT_ASSERT(img_info->h <= wincfg->h);

        yoffset  -= img_info->h / 2;
        xoffset  -= img_info->w / 2;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);
#endif
    }

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * Msg functions
 *
 **************************************************************************************************
 */

extern image_info_t msg_font_0_info;
extern image_info_t msg_font_1_info;
extern image_info_t msg_font_2_info;
extern image_info_t msg_font_percent_info;

extern image_info_t msg_font_CHN_6_info;
extern image_info_t msg_font_CHN_day_info;
extern image_info_t msg_font_CHN_month_info;
extern image_info_t msg_font_CN_week_info;

extern image_info_t msg_bat5_info;
extern image_info_t msg_mov3_info;

extern image_info_t msg_music3_info;
extern image_info_t msg_qq3_info;
extern image_info_t msg_wb3_info;
extern image_info_t msg_wx3_info;

/* 0 ~ 9 */
static const image_info_t *msg_font_num[10] =
{
    &msg_font_0_info,
    &msg_font_1_info,
    &msg_font_2_info,
    &msg_font_0_info,
    &msg_font_0_info,
    &msg_font_0_info,
    &msg_font_0_info,
    &msg_font_0_info,
    &msg_font_0_info,
    &msg_font_0_info,
};

static const image_info_t *msg_week_num[7] =
{
    &msg_font_CHN_day_info,
    &msg_font_CHN_day_info,
    &msg_font_CHN_day_info,
    &msg_font_CHN_day_info,
    &msg_font_CHN_day_info,
    &msg_font_CHN_day_info,
    &msg_font_CHN_6_info,
};

static const image_info_t *msg_bat_num[5] =
{
    &msg_bat5_info,
    &msg_bat5_info,
    &msg_bat5_info,
    &msg_bat5_info,
    &msg_bat5_info,
};

static const image_info_t *msg_music_num[3] =
{
    &msg_music3_info,
    &msg_music3_info,
    &msg_music3_info,
};

static const image_info_t *msg_qq_num[3] =
{
    &msg_qq3_info,
    &msg_qq3_info,
    &msg_qq3_info,
};

static const image_info_t *msg_web_num[3] =
{
    &msg_wb3_info,
    &msg_wb3_info,
    &msg_wb3_info,
};

static const image_info_t *msg_wx_num[3] =
{
    &msg_wx3_info,
    &msg_wx3_info,
    &msg_wx3_info,
};

/**
 * Msg region move.
 */
static rt_err_t olpc_clock_msg_region_move(struct olpc_clock_data *olpc_data)
{
    struct rt_device_graphic_info info;
    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    olpc_data->msg_xoffset += olpc_data->msg_xdir * MSG_MOVE_XSTEP;
    if (olpc_data->msg_xoffset > (MSG_REGION_W - MSG_FB_W) / 2)
    {
        olpc_data->msg_xoffset = (MSG_REGION_W - MSG_FB_W) / 2;
        olpc_data->msg_xdir = -1 * olpc_data->msg_xdir;
    }
    if (olpc_data->msg_xoffset <= (MSG_FB_W - MSG_REGION_W) / 2)
    {
        olpc_data->msg_xoffset = (MSG_FB_W - MSG_REGION_W) / 2;
        olpc_data->msg_xdir = -1 * olpc_data->msg_xdir;
    }

    olpc_data->msg_yoffset += olpc_data->msg_ydir * MSG_MOVE_YSTEP;
    if (olpc_data->msg_yoffset > (MSG_REGION_H - MSG_FB_H) / 2)
    {
        olpc_data->msg_yoffset = (MSG_REGION_H - MSG_FB_H) / 2;
        olpc_data->msg_ydir = -1 * olpc_data->msg_ydir;
    }
    if (olpc_data->msg_yoffset <= (MSG_FB_H - MSG_REGION_H) / 2)
    {
        olpc_data->msg_yoffset = (MSG_FB_H - MSG_REGION_H) / 2;
        olpc_data->msg_ydir = -1 * olpc_data->msg_ydir;
    }
    return RT_EOK;
}

/**
 * msg region refresh.
 */
static rt_err_t olpc_clock_msg_region_refresh(struct olpc_clock_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
    wincfg->winId  = CLOCK_RGB565_WIN;
    wincfg->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    wincfg->lut    = RT_NULL;
    wincfg->lutsize = 0;
#else
    wincfg->winId   = CLOCK_RGB332_WIN;
    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    wincfg->lut     = bpp_lut;
    wincfg->lutsize = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
#endif

    wincfg->fb    = olpc_data->fb;
    wincfg->w     = ((MSG_FB_W + 3) / 4) * 4;
    wincfg->h     = MSG_FB_H;
#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
    wincfg->fblen = wincfg->w * wincfg->h * 2;
#else
    wincfg->fblen = wincfg->w * wincfg->h;
#endif
    wincfg->x     = MSG_REGION_X + (MSG_REGION_W - MSG_FB_W) / 2 + olpc_data->msg_xoffset;
    wincfg->y     = MSG_REGION_Y + (MSG_REGION_H - MSG_FB_H) / 2 + olpc_data->msg_yoffset;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = olpc_data->msg_ylast;
    olpc_data->msg_ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->y % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    rt_memset((void *)wincfg->fb, 0x00, wincfg->fblen);

    xoffset = 0;
    yoffset = 0;

    // draw date & week
    uint16_t xoffset_bak = xoffset;
    uint16_t yoffset_bak = yoffset;

    yoffset = yoffset_bak + 0;
    {
        /* month num */
        xoffset = xoffset_bak + 49;
        img_info = (image_info_t *)msg_font_num[(olpc_data->month / 10) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        if ((olpc_data->month / 10) != 0)
        {
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);
        }
        xoffset  = xoffset + img_info->w + 0;
        img_info = (image_info_t *)msg_font_num[olpc_data->month % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* month */
        xoffset  = xoffset + img_info->w + 0;
        img_info = &msg_font_CHN_month_info;
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* day num */
        xoffset  = xoffset + img_info->w + 0;
        img_info = (image_info_t *)msg_font_num[(olpc_data->day / 10) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        xoffset  = xoffset + img_info->w + 0;
        img_info = (image_info_t *)msg_font_num[olpc_data->day % 10];
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* day */
        xoffset  = xoffset + img_info->w + 0;
        img_info = &msg_font_CHN_day_info;
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* week */
        xoffset  = xoffset + img_info->w + 30;
        img_info = &msg_font_CN_week_info;
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* week num */
        xoffset  = xoffset + img_info->w + 0;
        img_info = (image_info_t *)msg_week_num[olpc_data->week % 7];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);
    }

    /* draw battery & motion */
    yoffset = yoffset_bak + 80;
    {
        /* battary */
        xoffset = xoffset_bak + 48;
        img_info = (image_info_t *)msg_bat_num[olpc_data->batval / 20];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 8);

        /* battary num */
        xoffset += img_info->w + 0;
        img_info = (image_info_t *)msg_font_num[(olpc_data->batval / 100) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        if ((olpc_data->batval / 100) != 0)
        {
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);
        }

        xoffset += img_info->w + 0;
        img_info = (image_info_t *)msg_font_num[(olpc_data->batval / 10) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        if ((olpc_data->batval / 10) != 0)
        {
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);
        }

        xoffset += img_info->w + 0;
        img_info = (image_info_t *) msg_font_num[olpc_data->batval % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);

        xoffset += img_info->w;
        img_info = &msg_font_percent_info;
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);

        /* motion */
        xoffset += img_info->w + 30;
        img_info = &msg_mov3_info;
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 0);

        /* motion num */
        uint8_t flag = 0;
        xoffset += img_info->w + 0;
        img_info = (image_info_t *)msg_font_num[(olpc_data->moval / 100000) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        if (((olpc_data->moval / 100000) % 10) != 0)
        {
            flag = 1;
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);
            xoffset += img_info->w + 0;
        }
        img_info = (image_info_t *) msg_font_num[(olpc_data->moval / 10000) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        if (((olpc_data->moval / 10000) != 0) || (flag == 1))
        {
            flag = 1;
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);
            xoffset += img_info->w + 0;
        }
        img_info = (image_info_t *)msg_font_num[(olpc_data->moval / 1000) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        if (((olpc_data->moval / 1000) != 0) || (flag == 1))
        {
            flag = 1;
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);
            xoffset += img_info->w + 0;
        }
        img_info = (image_info_t *)msg_font_num[(olpc_data->moval / 100) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        if (((olpc_data->moval / 100) != 0) || (flag == 1))
        {
            flag = 1;
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);
            xoffset += img_info->w + 0;
        }
        img_info = (image_info_t *)msg_font_num[(olpc_data->moval / 10) % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        if (((olpc_data->moval / 10) != 0) || (flag == 1))
        {
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);
            xoffset += img_info->w + 0;
        }
        img_info = (image_info_t *)msg_font_num[olpc_data->moval % 10];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset + 3);
    }

    /* draw message: music & web & qq & wx */
    yoffset = yoffset_bak + 160;
    {
        /* music */
        xoffset = xoffset_bak + 106;
        img_info = (image_info_t *)msg_music_num[olpc_data->musicval];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* qq */
        xoffset += img_info->w + 20;
        img_info = (image_info_t *)msg_qq_num[olpc_data->qqval];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* web */
        xoffset += img_info->w + 20;
        img_info = (image_info_t *)msg_web_num[olpc_data->webval];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* wx */
        xoffset += img_info->w + 20;
        img_info = (image_info_t *)msg_wx_num[olpc_data->wexval];
        RT_ASSERT((yoffset + img_info->h) <= MSG_FB_H);
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);
    }

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * Home functions
 *
 **************************************************************************************************
 */

extern image_info_t home_backgroung_info;
extern image_info_t home_arrow1_info;
extern image_info_t home_lock1_info;
extern image_info_t home_unlock1_info;
extern image_info_t home_light1_info;
extern image_info_t home_cam1_info;
extern image_info_t home_srlock1_info;
extern image_info_t home_clock1_info;
extern image_info_t home_ebook1_info;
extern image_info_t home_game1_info;
extern image_info_t home_calc1_info;

static const image_info_t *home_item[8 + 1] =
{
    &home_unlock1_info,
    &home_light1_info,
    &home_cam1_info,
    &home_srlock1_info,
    &home_clock1_info,
    &home_ebook1_info,
    &home_game1_info,
    &home_calc1_info,
    RT_NULL,
};

/**
 * home region refresh.
 */
static rt_err_t olpc_clock_home_region_refresh(struct olpc_clock_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId  = CLOCK_GRAY1_WIN;
    wincfg->format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    wincfg->lut    = bpp1_lut;
    wincfg->lutsize = sizeof(bpp1_lut) / sizeof(bpp1_lut[0]);
    wincfg->fb    = olpc_data->fb;
    wincfg->w     = ((HOME_FB_W + 31) / 32) * 32;
    wincfg->h     = HOME_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = HOME_REGION_X + (HOME_REGION_W - HOME_FB_W) / 2;
    wincfg->y     = HOME_REGION_Y + (HOME_REGION_H - HOME_FB_H) / 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->y % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    rt_memset((void *)wincfg->fb, 0x00, wincfg->fblen);

    xoffset = 0;
    yoffset = 0;
    if (olpc_data->screen_sta == SCREEN_HOME)
    {
        /* home background */
        img_info = &home_backgroung_info;
        RT_ASSERT(img_info->w <= wincfg->w);
        RT_ASSERT(img_info->h <= wincfg->h);

        yoffset  += 0;
        xoffset  += (HOME_FB_W - img_info->w) / 2;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

        /* home arrow */
        img_info = &home_arrow1_info;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);

        /* home lock */
        img_info = &home_lock1_info;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);

        /* home ebook */
        img_info = (image_info_t *)home_item[olpc_data->home_id];
        if (img_info != NULL)
        {
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
        }
    }

    return RT_EOK;
}

#if defined(RT_USING_PISCES_TOUCH)
/**
 * home item touch.
 */
static rt_err_t olpc_clock_home_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->home_id = touch_id;
        olpc_data->cmd |= UPDATE_HOME;
        rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);
        ret = RT_EOK;
        break;

    case TOUCH_EVENT_UP:
        olpc_data->home_id = touch_id;
        rt_event_send(olpc_data->disp_event, EVENT_CLOCK_EXIT);
        ret = RT_EOK;
        break;

    default:
        break;
    }

    return ret;
}

static rt_err_t olpc_clock_home_touch_register(void *parameter)
{
    rt_int16_t   i, x, y;
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* home button key register */
    {
        x  = HOME_REGION_X + (HOME_REGION_W - HOME_FB_W) / 2;
        y  = HOME_REGION_Y + (HOME_REGION_H - HOME_FB_H) / 2;
        for (i = 0; i < 8; i++)
        {
            image_info_t *img_info = (image_info_t *)home_item[i];
            register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_clock_home_touch_callback, (void *)olpc_data, i);
            update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);
        }
    }

    return RT_EOK;
}

static rt_err_t olpc_clock_home_touch_unregister(void *parameter)
{
    rt_int16_t   i;

    /* home button key register */
    {
        for (i = 0; i < 8; i++)
        {
            image_info_t *img_info = (image_info_t *)home_item[i];
            unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));
        }
    }

    return RT_EOK;
}
#endif

/*
 **************************************************************************************************
 *
 * Fingerprint functions
 *
 **************************************************************************************************
 */

extern image_info_t fingerprint0_info;
extern image_info_t fingerprint1_info;
extern image_info_t fingerprint2_info;
extern image_info_t fingerprint3_info;

static const image_info_t *fingerprint_item[7] =
{
    &fingerprint3_info,
    &fingerprint2_info,
    &fingerprint1_info,
    &fingerprint0_info,
    &fingerprint1_info,
    &fingerprint2_info,
    &fingerprint3_info
};

/**
 * Fingerprint refresh timer callback.
 */
static void olpc_fp_timeout(void *parameter)
{
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;

    olpc_data->fp_id++;
    olpc_data->cmd |= UPDATE_FINGERP;
    rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);

    if (olpc_data->fp_id >= 6)
    {
        rt_timer_stop(olpc_data->fp_timer);
    }
}

/**
 * fingerprint region refresh.
 */
static rt_err_t olpc_clock_fingerprint_region_refresh(struct olpc_clock_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
    wincfg->winId  = CLOCK_RGB565_WIN;
    wincfg->format = RTGRAPHIC_PIXEL_FORMAT_RGB565;
    wincfg->lut    = RT_NULL;
    wincfg->lutsize = 0;
#else
    wincfg->winId   = CLOCK_RGB332_WIN;
    wincfg->format  = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    wincfg->lut     = bpp_lut;
    wincfg->lutsize = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
#endif
    wincfg->fb    = olpc_data->fb;
    wincfg->w     = ((FP_FB_W + 3) / 4) * 4;
    wincfg->h     = FP_FB_H;
#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
    wincfg->fblen = wincfg->w * wincfg->h * 2;
#else
    wincfg->fblen = wincfg->w * wincfg->h;
#endif
    wincfg->x     = FP_REGION_X + (FP_REGION_W - FP_FB_W) / 2;
    wincfg->y     = FP_REGION_Y + (FP_REGION_H - FP_FB_H) / 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    rt_memset((void *)wincfg->fb, 0x00, wincfg->fblen);

    xoffset = 0;
    yoffset = 0;

    if (olpc_data->screen_sta == SCREEN_HOME)
    {
        // draw fingerprint img
        img_info = (image_info_t *)fingerprint_item[olpc_data->fp_id];
        RT_ASSERT(img_info->w <= wincfg->w);
        RT_ASSERT(img_info->h <= wincfg->h);

        yoffset  += 0;
        xoffset  += (FP_FB_W - img_info->w) / 2;
        rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);
    }

    return RT_EOK;
}

#if defined(RT_USING_PISCES_TOUCH)
/**
 * fingerprint touch.
 */
static rt_err_t olpc_clock_fingerprint_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        olpc_data->fp_id  = 0;
        olpc_data->cmd |= UPDATE_FINGERP;
        rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);

        rt_timer_start(olpc_data->fp_timer);
        ret = RT_EOK;
        break;

    default:
        break;
    }

    return ret;
}

static rt_err_t olpc_clock_fingerprint_touch_register(void *parameter)
{
    rt_int16_t   x, y;
    image_info_t *img_info;
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* fingerprint button key register */
    {
        img_info = &fingerprint0_info;
        x  = FP_REGION_X + (FP_REGION_W - FP_FB_W) / 2 + (FP_FB_W - img_info->w) / 2;
        y  = FP_REGION_Y + (FP_REGION_H - FP_FB_H) / 2;
        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_clock_fingerprint_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);
    }

    return RT_EOK;
}

static rt_err_t olpc_clock_fingerprint_touch_unregister(void *parameter)
{
    image_info_t *img_info = &fingerprint0_info;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}
#endif

/*
 **************************************************************************************************
 *
 * LockBar functions
 *
 **************************************************************************************************
 */

extern image_info_t lock_bar_info;
extern image_info_t lock_block_info;

/**
 * lock region refresh.
 */
static rt_uint8_t unlock_flag = 0;
static rt_err_t olpc_clock_lock_region_refresh(struct olpc_clock_data *olpc_data,
        struct rt_display_config *wincfg)
{
    rt_err_t ret;
    rt_int16_t   xoffset, yoffset;
    rt_device_t  device = olpc_data->disp->device;
    image_info_t *img_info = NULL;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    wincfg->winId = CLOCK_GRAY1_WIN;
    wincfg->format = RTGRAPHIC_PIXEL_FORMAT_GRAY1;
    wincfg->lut  = bpp1_lut;
    wincfg->lutsize = sizeof(bpp1_lut) / sizeof(bpp1_lut[0]);
    wincfg->fb    = olpc_data->fb;
    wincfg->w     = ((LOCK_FB_W + 31) / 32) * 32;
    wincfg->h     = LOCK_FB_H;
    wincfg->fblen = wincfg->w * wincfg->h / 8;
    wincfg->x     = LOCK_REGION_X + (LOCK_REGION_W - LOCK_FB_W) / 2;
    wincfg->y     = LOCK_REGION_Y + (LOCK_REGION_H - LOCK_FB_H) / 2;
    wincfg->y     = (wincfg->y / 2) * 2;
    wincfg->ylast = wincfg->y;

    RT_ASSERT((wincfg->w % 4) == 0);
    RT_ASSERT((wincfg->y % 2) == 0);
    RT_ASSERT((wincfg->h % 2) == 0);
    RT_ASSERT(wincfg->fblen <= olpc_data->fblen);

    rt_memset((void *)wincfg->fb, 0x00, wincfg->fblen);

    xoffset = 0;
    yoffset = 0;

    if (olpc_data->screen_sta == SCREEN_LOCK)
    {
        if (unlock_flag == 0)
        {
            /* lock bar */
            img_info = &lock_bar_info;
            RT_ASSERT(img_info->w <= wincfg->w);
            RT_ASSERT(img_info->h <= wincfg->h);

            yoffset  += 0;
            xoffset  += (wincfg->w - img_info->w) / 2;
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset, yoffset);

            /* lock block */
            img_info = &lock_block_info;
            rt_int16_t xoff = (olpc_data->lock_px - (wincfg->x + xoffset + img_info->x));
            if (xoff < 0)
            {
                xoff = 0;
            }
            if (xoff > (wincfg->w - img_info->w - img_info->x - 8))
            {
                xoff = wincfg->w - img_info->w - img_info->x - 8;
                unlock_flag = 1;
            }
            xoffset += xoff;
            rt_display_img_fill(img_info, wincfg->fb, wincfg->w, xoffset + img_info->x, yoffset + img_info->y);
        }
        else
        {
            unlock_flag = 0;
            rt_memset((void *)wincfg->fb, 0x00, wincfg->fblen);
#if defined(RT_USING_PISCES_TOUCH)
            olpc_clock_lock_touch_unregister(olpc_data);
            olpc_clock_home_touch_register(olpc_data);
            olpc_clock_fingerprint_touch_register(olpc_data);
#endif
            olpc_data->home_id  = 8;
            olpc_data->fp_id    = 0;
            olpc_data->screen_sta = SCREEN_HOME;
            olpc_data->cmd = UPDATE_HOME | UPDATE_FINGERP;
            rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);
        }
    }

    return RT_EOK;
}

#if defined(RT_USING_PISCES_TOUCH)
/**
 * lock touch.
 */
static rt_err_t olpc_clock_lock_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    image_info_t *img = RT_NULL;
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    switch (event)
    {
    case TOUCH_EVENT_MOVE:
        img = &lock_bar_info;
        rt_uint16_t y  = LOCK_REGION_Y + (LOCK_REGION_H - LOCK_FB_H) / 2;
        if ((point->y <  y +  img->y - img->h) ||
                (point->y >  y + (img->y + img->h) + img->h))
        {
            img = &lock_block_info;
            olpc_data->lock_px = img->x_scr;
            olpc_data->lock_py = img->y_scr;
            olpc_touch_reset();
        }
        else
        {
            olpc_data->lock_px = point->x;
            olpc_data->lock_py = point->y;
        }

        olpc_data->cmd |= UPDATE_LOCK;
        rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);
        ret = RT_EOK;
        break;

    case TOUCH_EVENT_UP:
    {
        //reset lock state
        img = &lock_block_info;
        olpc_data->lock_px = img->x_scr;
        olpc_data->lock_py = img->y_scr;

        olpc_data->cmd |= UPDATE_LOCK;
        rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);
    }
    break;
    default:
        break;
    }

    return ret;
}

static rt_err_t olpc_clock_lock_touch_register(void *parameter)
{
    rt_int16_t   x, y;
    image_info_t *img_info;
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* fingerprint button key register */
    {
        img_info = &lock_block_info;
        x  = LOCK_REGION_X + (LOCK_REGION_W - LOCK_FB_W) / 2;
        y  = LOCK_REGION_Y + (LOCK_REGION_H - LOCK_FB_H) / 2;

        register_touch_item((struct olpc_touch_item *)(&img_info->touch_item), (void *)olpc_clock_lock_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&img_info->touch_item), x, y, img_info->x, img_info->y);
    }

    return RT_EOK;
}

static rt_err_t olpc_clock_lock_touch_unregister(void *parameter)
{
    image_info_t *img_info = &lock_block_info;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}
#endif

/*
 **************************************************************************************************
 *
 * olpc clock thread task funs
 *
 **************************************************************************************************
 */
#if defined(RT_USING_PISCES_TOUCH)
/**
 * screen touch.
 */
static image_info_t screen_item;
static rt_err_t olpc_clock_screen_touch_callback(rt_int32_t touch_id, enum olpc_touch_event event, struct point_info *point, void *parameter)
{
    rt_err_t ret = RT_ERROR;
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;

    switch (event)
    {
    case TOUCH_EVENT_SHORT_DOWN:
        if (olpc_data->screen_sta == SCREEN_OFF)
        {
            olpc_data->screen_sta = SCREEN_LOCK;

            olpc_clock_lock_touch_register(parameter);

            image_info_t *img = &lock_block_info;
            olpc_data->lock_px = img->x_scr;
            olpc_data->lock_py = img->y_scr;
            olpc_data->cmd |= UPDATE_LOCK;
            rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);


            ret = RT_EOK;
        }
        break;

    default:
        break;
    }

    rt_timer_start(olpc_data->src_timer);   // start & restart timer

    return ret;
}

static rt_err_t olpc_clock_screen_touch_register(void *parameter)
{
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;
    struct rt_device_graphic_info info;

    rt_memcpy(&info, &olpc_data->disp->info, sizeof(struct rt_device_graphic_info));

    /* screen on button key register */
    {
        screen_item.w = WIN_LAYERS_W;
        screen_item.h = WIN_LAYERS_H;
        register_touch_item((struct olpc_touch_item *)(&screen_item.touch_item), (void *)olpc_clock_screen_touch_callback, (void *)olpc_data, 0);
        update_item_coord((struct olpc_touch_item *)(&screen_item.touch_item), 0, 0, 0, 0);
    }

    return RT_EOK;
}

static rt_err_t olpc_clock_screen_touch_unregister(void *parameter)
{
    image_info_t *img_info = &screen_item;

    unregister_touch_item((struct olpc_touch_item *)(&img_info->touch_item));

    return RT_EOK;
}

#endif

/**
 * Screen timeout timer callback.
 */
static void olpc_screen_timeout(void *parameter)
{
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;

    if (olpc_data->screen_sta == SCREEN_HOME)
    {
#if defined(RT_USING_PISCES_TOUCH)
        olpc_clock_home_touch_unregister(olpc_data);
        olpc_clock_fingerprint_touch_unregister(olpc_data);
#endif
        olpc_data->screen_sta = SCREEN_OFF;
        olpc_data->cmd |= UPDATE_HOME | UPDATE_FINGERP;
        rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);
    }
    else if (olpc_data->screen_sta == SCREEN_LOCK)
    {
#if defined(RT_USING_PISCES_TOUCH)
        olpc_clock_lock_touch_unregister(olpc_data);
#endif
        olpc_data->screen_sta = SCREEN_OFF;
        olpc_data->cmd |= UPDATE_LOCK;
        rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);
    }

    olpc_data->screen_sta = SCREEN_OFF;
}

/**
 * Display clock demo init.
 */
static rt_err_t olpc_clock_init(struct olpc_clock_data *olpc_data)
{
    rt_err_t    ret;
    rt_device_t device = olpc_data->disp->device;
    struct rt_device_graphic_info info;

    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->hour = 3;
    olpc_data->minute = 30;
    olpc_data->second = 0;
    olpc_data->ticks = 0;
    olpc_data->day = 2;
    olpc_data->month = 12;
    olpc_data->week = 6;

    olpc_data->batval = 21;
    olpc_data->moval = 100121;
    olpc_data->musicval = 2;
    olpc_data->qqval = 2;
    olpc_data->webval = 2;
    olpc_data->wexval = 2;

    olpc_data->clock_xoffset = 0;
    olpc_data->clock_yoffset = 0;
    olpc_data->clock_xdir = -1;
    olpc_data->clock_ydir = -1;
    olpc_data->clock_ylast = CLOCK_REGION_Y + (CLOCK_REGION_H - CLOCK_FB_H) / 2;

    olpc_data->msg_xoffset = 0;
    olpc_data->msg_yoffset = 0;
    olpc_data->msg_xdir = 1;
    olpc_data->msg_ydir = 1;
    olpc_data->msg_ylast = MSG_REGION_Y + (MSG_REGION_H - MSG_FB_H) / 2;

    olpc_data->home_id = 0;
    olpc_data->fp_id = 0;
    olpc_data->screen_sta = SCREEN_OFF;

    olpc_data->cmd = UPDATE_CLOCK | UPDATE_MSG;

#if defined(OLPC_APP_CLOCK_STYLE_ROUND_ROMAN565)
    olpc_data->fblen = CLOCK_FB_W * CLOCK_FB_H * 2;
#else
    olpc_data->fblen = CLOCK_FB_W * CLOCK_FB_H;
#endif
    olpc_data->fb    = (rt_uint8_t *)rt_malloc_large(olpc_data->fblen);
    RT_ASSERT(olpc_data->fb != RT_NULL);

#if defined(OLPC_CLOCKBKG_USE_DTCM)
    image_info_t *img_info = &clock_bkg_info;
    rt_uint16_t xVir    = ((CLOCK_FB_W + 3) / 4) * 4;
    rt_uint16_t yoffset = (CLOCK_FB_H - img_info->h) / 2;//0;
    rt_uint16_t xoffset = (CLOCK_FB_W - img_info->w) / 2;

    olpc_data->bkgfblen = CLOCK_FB_W * CLOCK_FB_H * 2;
    olpc_data->bkgfb    = (rt_uint8_t *)rt_malloc_dtcm(olpc_data->bkgfblen);
    RT_ASSERT(olpc_data->bkgfb != RT_NULL);
    rt_memset(olpc_data->bkgfb, 0, olpc_data->bkgfblen);
    rt_display_img_fill(img_info, olpc_data->bkgfb, xVir, xoffset, yoffset);
#endif

    // create sreen touch timer
    olpc_data->src_timer = rt_timer_create("srctimer",
                                           olpc_screen_timeout,
                                           (void *)olpc_data,
                                           SCREEN_HOLD_TIME,
                                           RT_TIMER_FLAG_ONE_SHOT);
    RT_ASSERT(olpc_data->src_timer != RT_NULL);

    // create fp touch timer
    olpc_data->fp_timer = rt_timer_create("fptimer",
                                          olpc_fp_timeout,
                                          (void *)olpc_data,
                                          FP_LOOP_TIME,
                                          RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->fp_timer != RT_NULL);

    // create clock timer
    olpc_data->clock_timer = rt_timer_create("clocktimer",
                             olpc_clock_timeout,
                             (void *)olpc_data,
                             CLOCK_UPDATE_TIME,
                             RT_TIMER_FLAG_PERIODIC);
    RT_ASSERT(olpc_data->clock_timer != RT_NULL);
    rt_timer_start(olpc_data->clock_timer);

#if defined(RT_USING_PISCES_TOUCH)
    olpc_clock_screen_touch_register(olpc_data);
#endif

    return RT_EOK;
}

/**
 * Display clock demo deinit.
 */
static void olpc_clock_deinit(struct olpc_clock_data *olpc_data)
{
    rt_err_t ret;

#if defined(RT_USING_PISCES_TOUCH)
    olpc_clock_screen_touch_unregister(olpc_data);
    olpc_touch_list_clear();
#endif
    rt_timer_stop(olpc_data->fp_timer);
    ret = rt_timer_delete(olpc_data->fp_timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->fp_timer = RT_NULL;

    rt_timer_stop(olpc_data->src_timer);
    ret = rt_timer_delete(olpc_data->src_timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->src_timer = RT_NULL;

    ret = rt_timer_stop(olpc_data->clock_timer);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_timer_delete(olpc_data->clock_timer);
    RT_ASSERT(ret == RT_EOK);
    olpc_data->clock_timer = RT_NULL;

#if defined(OLPC_CLOCKBKG_USE_DTCM)
    rt_free_dtcm((void *)olpc_data->bkgfb);
    olpc_data->bkgfb = RT_NULL;
#endif
    rt_free_large((void *)olpc_data->fb);
    olpc_data->fb = RT_NULL;
}

static rt_err_t olpc_clock_disp_finish(void)
{
    rt_err_t ret;

    ret = rt_event_send(g_olpc_data->disp_event, EVENT_CLOCK_DISP);

    return ret;
}

static rt_err_t olpc_clock_disp_wait(void)
{
    rt_err_t ret;
    uint32_t event;

    ret = rt_event_recv(g_olpc_data->disp_event, EVENT_CLOCK_DISP,
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                        RT_WAITING_FOREVER, &event);
    RT_ASSERT(ret == RT_EOK);

    return RT_EOK;
}

/**
 * Display clock demo task function.
 */
static rt_err_t olpc_clock_task_fun(struct olpc_clock_data *olpc_data)
{
    rt_err_t    ret;
    rt_uint32_t updatecmd;
    struct rt_display_mq_t disp_mq;

    updatecmd = olpc_data->cmd;
    olpc_data->cmd = 0;

    //rt_tick_t ticks = rt_tick_get();
    do
    {
        rt_memset(&disp_mq, 0, sizeof(struct rt_display_mq_t));
        disp_mq.win[0].zpos = WIN_TOP_LAYER;
        disp_mq.win[1].zpos = WIN_BOTTOM_LAYER;

        // clock & msg move process
        {
            if ((updatecmd & MOVE_CLOCK) == MOVE_CLOCK)
            {
                updatecmd &= ~MOVE_CLOCK;

                olpc_clock_clock_region_move(olpc_data);
            }

            if ((updatecmd & MOVE_MSG) == MOVE_MSG)
            {
                updatecmd &= ~MOVE_MSG;

                olpc_clock_msg_region_move(olpc_data);
            }
        }

        // display process
        {
            if ((updatecmd & UPDATE_CLOCK) == UPDATE_CLOCK)
            {
                updatecmd &= ~UPDATE_CLOCK;
#if defined(OLPC_CLOCKBKG_USE_DTCM)
                ret = olpc_clock_clock_bkg_refresh(olpc_data, &disp_mq.win[1]);
                RT_ASSERT(ret == RT_EOK);
                disp_mq.cfgsta |= (0x01 << 1);
#endif
                ret = olpc_clock_clock_region_refresh(olpc_data, &disp_mq.win[0]);
                RT_ASSERT(ret == RT_EOK);
                disp_mq.cfgsta |= (0x01 << 0);
            }
            else if ((updatecmd & UPDATE_MSG) == UPDATE_MSG)
            {
                updatecmd &= ~UPDATE_MSG;

                ret = olpc_clock_msg_region_refresh(olpc_data, &disp_mq.win[0]);
                RT_ASSERT(ret == RT_EOK);
                disp_mq.cfgsta |= (0x01 << 0);
            }
            else if ((updatecmd & UPDATE_HOME) == UPDATE_HOME)
            {
                updatecmd &= ~UPDATE_HOME;

                ret = olpc_clock_home_region_refresh(olpc_data, &disp_mq.win[0]);
                RT_ASSERT(ret == RT_EOK);
                disp_mq.cfgsta |= (0x01 << 0);
            }
            else if ((updatecmd & UPDATE_FINGERP) == UPDATE_FINGERP)
            {
                updatecmd &= ~UPDATE_FINGERP;

                ret = olpc_clock_fingerprint_region_refresh(olpc_data, &disp_mq.win[0]);
                RT_ASSERT(ret == RT_EOK);
                disp_mq.cfgsta |= (0x01 << 0);
            }
            else if ((updatecmd & UPDATE_LOCK) == UPDATE_LOCK)
            {
                updatecmd &= ~UPDATE_LOCK;

                ret = olpc_clock_lock_region_refresh(olpc_data, &disp_mq.win[0]);
                RT_ASSERT(ret == RT_EOK);
                disp_mq.cfgsta |= (0x01 << 0);
            }

            if (disp_mq.cfgsta)
            {
                disp_mq.disp_finish = olpc_clock_disp_finish;
                ret = rt_mq_send(olpc_data->disp->disp_mq, &disp_mq, sizeof(struct rt_display_mq_t));
                RT_ASSERT(ret == RT_EOK);
                olpc_clock_disp_wait();
            }
        }
    }
    while (updatecmd != 0);

    //rt_kprintf("clock ticks = %d\n", rt_tick_get() - ticks);

    return RT_EOK;
}

/*
 **************************************************************************************************
 *
 * olpc clock demo init & thread
 *
 **************************************************************************************************
 */

/**
 * olpc clock lut set.
 */
static rt_err_t olpc_clock_display_clear(void *parameter)
{
    rt_err_t ret = RT_EOK;
    struct rt_display_lut lut0;
    struct rt_device_graphic_info info;
    struct olpc_clock_data *olpc_data = (struct olpc_clock_data *)parameter;

    lut0.winId = CLOCK_RGB332_WIN;
    lut0.format = RTGRAPHIC_PIXEL_FORMAT_RGB332;
    lut0.lut  = bpp_lut;
    lut0.size = sizeof(bpp_lut) / sizeof(bpp_lut[0]);
    ret = rt_display_lutset(&lut0, 0, 0);
    RT_ASSERT(ret == RT_EOK);

    rt_device_t device = olpc_data->disp->device;
    ret = rt_device_control(device, RTGRAPHIC_CTRL_GET_INFO, &info);
    RT_ASSERT(ret == RT_EOK);

    rt_display_win_clear(CLOCK_RGB332_WIN, RTGRAPHIC_PIXEL_FORMAT_RGB332, 0, WIN_LAYERS_H, 0);

    return RT_EOK;
}

/**
 * event table for task switch to.
 */
#define OLPC_CLOCK_HOME_ID_MAX  8
static rt_uint32_t task_switch_table[OLPC_CLOCK_HOME_ID_MAX] =
{
    // 0
#if defined(OLPC_APP_LYRIC_ENABLE)
    EVENT_APP_LYRIC,
#else
    0,
#endif
    // 1
#if defined(OLPC_APP_BLN_ENABLE)
    EVENT_APP_BLN,
#else
    0,
#endif
    // 2
#if defined(OLPC_APP_XSCREEN_ENABLE)
    EVENT_APP_XSCREEN,
#else
    0,
#endif
    // 3
#if defined(OLPC_APP_NOTE_ENABLE)
    EVENT_APP_NOTE,
#else
    0,
#endif
    // 4
#if defined(OLPC_APP_SNAKE_ENABLE)
    EVENT_APP_SNAKE,
#else
    0,
#endif
    // 5
#if defined(OLPC_APP_EBOOK_ENABLE)
    EVENT_APP_EBOOK,
#else
    0,
#endif
    // 6
#if defined(OLPC_APP_BLOCK_ENABLE)
    EVENT_APP_BLOCK,
#else
    0,
#endif
    // 7
#if defined(OLPC_APP_JUPITER_ENABLE)
    EVENT_APP_JUPITER,
#else
    0,
#endif
};

/**
 * olpc clock dmeo thread.
 */
static void olpc_clock_thread(void *p)
{
    rt_err_t ret;
    uint32_t event, exitevt = EVENT_APP_CLOCK;
    struct olpc_clock_data *olpc_data;

    olpc_data = (struct olpc_clock_data *)rt_malloc(sizeof(struct olpc_clock_data));
    RT_ASSERT(olpc_data != RT_NULL);
    rt_memset((void *)olpc_data, 0, sizeof(struct olpc_clock_data));
    g_olpc_data = olpc_data;

    olpc_data->disp = rt_display_get_disp();
    RT_ASSERT(olpc_data->disp != RT_NULL);

    ret = olpc_clock_display_clear(olpc_data);
    RT_ASSERT(ret == RT_EOK);

    olpc_data->disp_event = rt_event_create("display_event", RT_IPC_FLAG_FIFO);
    RT_ASSERT(olpc_data->disp_event != RT_NULL);

    ret = olpc_clock_init(olpc_data);
    RT_ASSERT(ret == RT_EOK);

#if defined(OLPC_APP_CLOCK_BLN_ENABLE)
    rt_thread_t  rtt_bln = rt_thread_create("olpbln", olpc_bln_thread, RT_NULL, 2048, 4, 10);
    RT_ASSERT(rtt_bln != RT_NULL);
    rt_thread_startup(rtt_bln);
#endif

    rt_event_send(olpc_data->disp_event, EVENT_CLOCK_UPDATE);
    while (1)
    {
        ret = rt_event_recv(olpc_data->disp_event, EVENT_CLOCK_UPDATE | EVENT_CLOCK_EXIT,
                            RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                            RT_WAITING_FOREVER, &event);
        if (ret != RT_EOK)
        {
            /* Reserved... */
        }

        if (event & EVENT_CLOCK_UPDATE)
        {
            ret = olpc_clock_task_fun(olpc_data);
            RT_ASSERT(ret == RT_EOK);
        }

        if (event & EVENT_CLOCK_EXIT)
        {
            RT_ASSERT(olpc_data->home_id < OLPC_CLOCK_HOME_ID_MAX);
            exitevt = task_switch_table[olpc_data->home_id];
            if (exitevt != 0)
            {
                break;
            }
        }
    }

#if defined(OLPC_APP_CLOCK_BLN_ENABLE)
    ret = rt_event_send(olpc_data->disp_event, EVENT_BLN_EXIT);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_event_recv(olpc_data->disp_event, EVENT_BLN_EXIT_DONE,
                        RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                        RT_WAITING_FOREVER, &event);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_thread_delete(rtt_bln);
    RT_ASSERT(ret == RT_EOK);
#endif

    olpc_clock_deinit(olpc_data);

    rt_event_delete(olpc_data->disp_event);
    olpc_data->disp_event = RT_NULL;

    rt_free(olpc_data);
    olpc_data = RT_NULL;

    rt_event_send(olpc_main_event, exitevt);
}

/**
 * olpc clock demo application init.
 */
#if defined(OLPC_DLMODULE_ENABLE)
SECTION(".param") rt_uint16_t dlmodule_thread_priority = 5;
SECTION(".param") rt_uint32_t dlmodule_thread_stacksize = 2048;
int main(int argc, char *argv[])
{
    olpc_clock_thread(RT_NULL);
    return RT_EOK;
}

#else
int olpc_clock_app_init(void)
{
    rt_thread_t  rtt_clock = rt_thread_create("olpclk", olpc_clock_thread, RT_NULL, 2048, 5, 10);
    RT_ASSERT(rtt_clock != RT_NULL);
    rt_thread_startup(rtt_clock);

    return RT_EOK;
}
//INIT_APP_EXPORT(olpc_clock_app_init);
#endif

#endif
