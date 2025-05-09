/**
  * Copyright (c) 2020 Rockchip Electronics Co., Ltd
  *
  * SPDX-License-Identifier: Apache-2.0
  */

#include <rtthread.h>
#include <stdio.h>
#include <string.h>

#include "drv_dsp.h"
#include "drv_heap.h"
#include "hal_base.h"
#include "rk_audio.h"

#include "lib_imageprocess.h"
#include "app_asr.h"

#define ASR_WAKE_ID                  0x50000002
#define ASR_WAKEUP_CONFIG            0x40000003
#define ASR_WAKEUP_START             0x40000004

#define ASR_BUF_LEN                  1024

#define EVENT_START     (0x1U << 1)
#define EVENT_STOP      (0x1U << 2)
#define EVENT_PAUSE     (0x1U << 3)
#define EVENT_EXIT      (0x1U << 4)
#define EVENT_IDLE      (0x1U << 5)

#define SHOW_TICK       0

#define ENABLE_ASR      0

#if ENABLE_ASR

struct wake_work_param
{
    uint32_t buf;
    uint32_t buf_size;
    uint32_t out_buf;
    uint32_t result;

    uint32_t aec_buf;
};

static struct rt_device *g_dsp_dev = NULL;
static struct rt_device *g_card = NULL;
static struct dsp_work *g_work = NULL;
static struct wake_work_param *g_param = NULL;
static struct AUDIO_PARAMS aparam;
static struct audio_buf abuf;

static struct rt_event event;
static rt_sem_t asr_pause = NULL;

static int app_asr_state;
static asr_cb app_asr_callback = NULL;

static int asr_init(void)
{
    uint32_t size;
    rt_err_t ret;

#if RK_ROTATE_USING_DSP || RK_SCALE_USING_DSP || RK_LZW_USING_DSP
    g_dsp_dev = (struct rt_device *)rk_imagelib_dsp_dev();
#else
    uint32_t rate;

    g_dsp_dev = rt_device_find("dsp0");
    if (g_dsp_dev == NULL)
    {
        rt_kprintf("Cannot find dsp0\n");
        list_device();
        return -RT_ERROR;
    }
    rt_device_open(g_dsp_dev, RT_DEVICE_OFLAG_RDWR);

    rt_device_control(g_dsp_dev, RKDSP_CTL_SET_FREQ, (void *)(396000000));
    rt_device_control(g_dsp_dev, RKDSP_CTL_GET_FREQ, (void *)&rate);
    // rt_kprintf("Current dsp freq: %d MHz\n", rate / 1000000);
#endif

    g_work = (struct dsp_work *)rkdsp_malloc(sizeof(struct dsp_work));
    RT_ASSERT(g_work != NULL);
    memset(g_work, 0, sizeof(struct dsp_work));
    g_work->id = ASR_WAKE_ID;
    g_work->algo_type = ASR_WAKEUP_CONFIG;
    ret = rt_device_control(g_dsp_dev, RKDSP_CTL_QUEUE_WORK, g_work);
    RT_ASSERT(!ret);
    ret = rt_device_control(g_dsp_dev, RKDSP_CTL_DEQUEUE_WORK, g_work);
    RT_ASSERT(!ret);

    g_param = (struct wake_work_param *)rkdsp_malloc(sizeof(struct wake_work_param));
    RT_ASSERT(g_param != NULL);
    memset((void *)g_param, 0x0, sizeof(struct wake_work_param));
    g_param->buf = (uint32_t)rkdsp_malloc(ASR_BUF_LEN);
    RT_ASSERT(g_param->buf != NULL);
    g_param->buf_size = ASR_BUF_LEN;
    g_param->result = 0;

    g_work->algo_type = ASR_WAKEUP_START;
    g_work->param = (uint32_t)g_param;
    g_work->param_size = sizeof(struct wake_work_param);

    g_card = rt_device_find("pdmc");
    RT_ASSERT(g_card != NULL);
    ret = rt_device_open(g_card, RT_DEVICE_OFLAG_RDONLY);
    RT_ASSERT(ret == RT_EOK);

    abuf.period_size = ASR_BUF_LEN / (2 * (16 >> 3));
    abuf.buf_size = abuf.period_size * 4;
    size = abuf.buf_size * 2 * (16 >> 3); /* frames to bytes */
    abuf.buf = rt_malloc_uncache(size);
    RT_ASSERT(abuf.buf != NULL);

    aparam.channels = 2;
    aparam.sampleRate = 16000;
    aparam.sampleBits = 16;

    ret = rt_device_control(g_card, RK_AUDIO_CTL_PCM_PREPARE, &abuf);
    RT_ASSERT(ret == RT_EOK);

    ret = rt_device_control(g_card, RK_AUDIO_CTL_HW_PARAMS, &aparam);
    RT_ASSERT(ret == RT_EOK);

    return RT_EOK;
}

#if SHOW_TICK
int cnt = 0;
uint32_t cycles = 0;
#endif
static int asr_process(void)
{
    short *buffer = (short *)g_param->buf;
    rt_err_t ret;

    rt_device_read(g_card, 0, buffer, abuf.period_size);
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, (void *)buffer, ASR_BUF_LEN);

    ret = rt_device_control(g_dsp_dev, RKDSP_CTL_QUEUE_WORK, g_work);
    RT_ASSERT(!ret);
    ret = rt_device_control(g_dsp_dev, RKDSP_CTL_DEQUEUE_WORK, g_work);
    RT_ASSERT(!ret);

#if SHOW_TICK
    cycles += g_work->cycles;
    cnt++;
    if (cnt > (16000 / 256))
    {
        rt_kprintf("cycles %lu\n", cycles);
        cycles = 0;
        cnt = 0;
    }
#endif

    return g_work->result;
}

static int asr_deinit(void)
{
    rt_err_t ret;

    ret = rt_device_control(g_card, RK_AUDIO_CTL_STOP, NULL);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(g_card, RK_AUDIO_CTL_PCM_RELEASE, NULL);
    RT_ASSERT(ret == RT_EOK);

    rt_free_uncache(abuf.buf);
    rt_device_close(g_card);

    rkdsp_free((void *)g_param->buf);
    rkdsp_free(g_param);
    rkdsp_free(g_work);

#if !RK_ROTATE_USING_DSP && !RK_SCALE_USING_DSP && !RK_LZW_USING_DSP
    rt_device_close(g_dsp_dev);
#endif

    return RT_EOK;
}

static void app_asr_run(void *arg)
{
    int ret;

    while (app_asr_state == EVENT_START ||
            app_asr_state == EVENT_PAUSE)
    {
        ret = asr_process();
        if (ret)
        {
            if (app_asr_callback)
            {
                app_asr_callback();
            }
        }
        if (app_asr_state == EVENT_PAUSE)
        {
            rt_sem_take(asr_pause, RT_WAITING_FOREVER);
        }
    }
    if (app_asr_state == EVENT_EXIT)
        asr_deinit();
    app_asr_state = EVENT_IDLE;
}

static void app_asr_handle(void *arg)
{
    rt_thread_t thread = NULL;
    rt_uint32_t e;

    while (1)
    {
        rt_event_recv(&event, (EVENT_STOP | EVENT_START | EVENT_EXIT),
                      RT_EVENT_FLAG_OR | RT_EVENT_FLAG_CLEAR,
                      RT_WAITING_FOREVER, &e);
        if (e & EVENT_START)
        {
            if (thread == NULL)
            {
                app_asr_state = EVENT_START;
                thread = rt_thread_create("asr", app_asr_run, RT_NULL, 4096, 5, 10);
                RT_ASSERT(thread != RT_NULL);
                rt_thread_startup(thread);
            }
        }
        if (e & EVENT_STOP)
        {
            if (thread != NULL)
            {
                app_asr_state = EVENT_STOP;
                thread = NULL;
            }
            else
            {
                app_asr_state = EVENT_IDLE;
            }
        }
        if (e & EVENT_EXIT)
        {
            app_asr_state = EVENT_EXIT;
            if (thread == NULL)
                asr_deinit();
            break;
        }
    }
    rt_event_detach(&event);
    rt_sem_delete(asr_pause);
}

void app_asr_set_callback(asr_cb cb)
{
    app_asr_callback = cb;
}

void app_asr_start(void)
{
    rt_event_send(&event, EVENT_START);
}

void app_asr_stop(void)
{
    int timeout = 100;

    rt_event_send(&event, EVENT_STOP);

    while (app_asr_state != EVENT_IDLE)
    {
        rt_thread_mdelay(10);
        timeout--;
        if (timeout <= 0)
            break;
    }
}

void app_asr_pause(void)
{
    app_asr_state = EVENT_PAUSE;
    rt_thread_mdelay(100);
}

void app_asr_resume(void)
{
    app_asr_state = EVENT_START;
    rt_sem_release(asr_pause);
}

void app_asr_deinit(void)
{
    rt_event_send(&event, EVENT_EXIT);
}

void app_asr_init(void)
{
    rt_thread_t thread;

    asr_init();
    rt_event_init(&event, "asr", RT_IPC_FLAG_FIFO);
    asr_pause = rt_sem_create("asr", 1, RT_IPC_FLAG_PRIO);

    thread = rt_thread_create("asrtask", app_asr_handle, RT_NULL, 4096, 5, 10);
    RT_ASSERT(thread != RT_NULL);

    rt_thread_startup(thread);

    app_asr_start();
}
#else
void app_asr_set_callback(asr_cb cb) {};
void app_asr_start(void) {};
void app_asr_stop(void) {};
void app_asr_pause(void) {};
void app_asr_resume(void) {};
void app_asr_deinit(void) {};
void app_asr_init(void) {};
#endif
