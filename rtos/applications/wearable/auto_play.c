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

#ifdef APP_AUTO_PLAY

struct auto_play_s
{
    rt_sem_t sem;
} auto_play;

typedef rt_err_t(*auto_job)(void *arg);
struct auto_play_job
{
    auto_job job;
    void *arg;
    int delay;
};

extern design_cb_t clock_move_lr_design;
extern design_cb_t clock_move_updn_design;
extern design_cb_t message_move_updn_design;
extern rt_err_t app_charging_touch_up(void *param);
extern rt_err_t app_funclist_touch_up(void *param);

static int slide_index = 0;
static mov_design_param auto_play_mov_design_param;
rt_err_t page_slide(void *arg)
{
    auto_play_mov_design_param.dir = slide_index < 3 ? 1 : -1;
    auto_play_mov_design_param.offset = CLOCK_WIN_XRES * (app_main_page_data->cur_page + auto_play_mov_design_param.dir);

    app_design_request(0, &clock_move_lr_design, &auto_play_mov_design_param);

    slide_index++;
    if (slide_index >= 7)
        slide_index = 0;

    auto_play_next(NULL);

    return RT_EOK;
}

static int updn_dir = 1;
rt_err_t updown_slide(void *arg)
{
    auto_play_mov_design_param.dir = updn_dir;
    auto_play_mov_design_param.offset = MSG_WIN_YRES * auto_play_mov_design_param.dir;

    app_design_request(0, &clock_move_updn_design, &auto_play_mov_design_param);

    updn_dir = -updn_dir;

    auto_play_next(NULL);

    return RT_EOK;
}

rt_err_t msg_updown_slide(void *arg)
{
    auto_play_mov_design_param.dir = updn_dir;
    auto_play_mov_design_param.offset = 0;

    app_design_request(0, &message_move_updn_design, &auto_play_mov_design_param);

    auto_play_next(NULL);

    return RT_EOK;
}

rt_err_t charging_start(void *arg)
{
    app_charging_enable(NULL);

    auto_play_next(NULL);

    return RT_EOK;
}

rt_err_t charging_end(void *arg)
{
    app_charging_touch_up(NULL);

    auto_play_next(NULL);

    return RT_EOK;
}

struct rt_touch_data point[1];
rt_err_t funclist_enter(void *arg)
{
    app_funclist_page_show_funclist(point);

    auto_play_next(NULL);

    return RT_EOK;
}

rt_err_t funclist_exit(void *arg)
{
    app_funclist_touch_up(NULL);

    auto_play_next(NULL);

    return RT_EOK;
}

struct auto_play_job jobs[] =
{
    {page_slide, NULL, 2000},
    {page_slide, NULL, 2000},
    {page_slide, NULL, 2000},
    {page_slide, NULL, 2000},
    {updown_slide, NULL, 2000},
    {msg_updown_slide, NULL, 5000},
    {charging_start, NULL, 2000},
    {charging_end, NULL, 10000},
    {updown_slide, NULL, 2000},
    {funclist_enter, NULL, 2000},
    {funclist_exit, NULL, 2000},
    {msg_updown_slide, NULL, 2000},
};

rt_err_t auto_play_next(void *arg)
{
    rt_sem_release(auto_play.sem);

    return RT_EOK;
}

rt_err_t auto_play_wait(void *arg)
{
    rt_sem_take(auto_play.sem, RT_WAITING_FOREVER);

    return RT_EOK;
}

uint32_t GetRandom(uint32_t range)
{
    uint32_t seed;
    uint32_t val;
    seed = rt_tick_get();

    srand(seed);
    val = rand() % range;
    if (val == 0)
        val = 1;

    return val;
}

void auto_play_run(void *arg)
{
    int job_index = 0;

    while (1)
    {
        auto_play_wait(NULL);

        app_main_register_timeout_cb(jobs[job_index].job, jobs[job_index].arg, jobs[job_index].delay);

        /* touch point random or center */
#if 1
        point[0].x_coordinate = GetRandom(DISP_XRES);
        point[0].y_coordinate = GetRandom(DISP_YRES);
#else
        point[0].x_coordinate = DISP_XRES / 2;
        point[0].y_coordinate = DISP_YRES / 2;
#endif

        job_index++;
        if (job_index >= (sizeof(jobs) / sizeof(jobs[0])))
            job_index = 0;
    }
}

rt_err_t auto_play_init(void *arg)
{
    rt_thread_t thread;

    auto_play.sem = rt_sem_create("autoplay", 1, RT_IPC_FLAG_PRIO);
    RT_ASSERT(auto_play.sem != RT_NULL);

    thread = rt_thread_create("autoplay", auto_play_run, RT_NULL, 8192, 5, 10);
    RT_ASSERT(thread != RT_NULL);

    rt_thread_startup(thread);

    auto_play_next(NULL);

    return RT_EOK;
}
#endif
