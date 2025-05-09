/*
 * Copyright (c) 2021 Fuzhou Rockchip Electronic Co.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2021-03-12     Jair Wu      First version
 *
 */

#include "AudioConfig.h"
#ifdef __RT_THREAD__

#include "dsd_common.h"

#define DSP_CODEC_DSD_DECODE_OPEN       0x5000000F
#define DSP_CODEC_DSD_DECODE_PROCESS    0x50000010
#define DSP_CODEC_DSD_DECODE_CLOSE      0x50000011

struct dsd2pcm
{
    void *converter;
    int type;
    int channels;
    int dsd_samplerate;
    int pcm_samplerate;

    uint8_t *dsd_data;
    uint32_t dsd_size;
    uint8_t *pcm_out;
    uint16_t bps;
    int dsf;

    struct rt_device *dsp;
    struct dsp_work *work;
};

extern void *get_dsp_fw_by_id(int id);

void *dsd2pcm_converter_init(dec_type type, int channels, int dsd_samplerate, int pcm_samplerate)
{
    struct dsd2pcm *dsd2pcm;
    int ret;

    dsd2pcm = rkdsp_malloc(sizeof(struct dsd2pcm));
    RT_ASSERT(dsd2pcm != NULL);

    dsd2pcm->dsp = rt_device_find("dsp0");
    RT_ASSERT(dsd2pcm->dsp != NULL);
    rt_device_control(dsd2pcm->dsp, RKDSP_CTL_PREPARE_IMAGE_DATA, (void *)get_dsp_fw_by_id(0));
    ret = rt_device_open(dsd2pcm->dsp, RT_DEVICE_OFLAG_RDWR);
    RT_ASSERT(ret == RT_EOK);

#ifdef RT_USING_PM_DVFS
    uint32_t freq;
    ret = rt_device_control(dsd2pcm->dsp, RKDSP_CTL_SET_FREQ, (void *)297000000);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(dsd2pcm->dsp, RKDSP_CTL_GET_FREQ, (void *)&freq);
    RT_ASSERT(ret == RT_EOK);
    RK_AUDIO_LOG_V("DSP set freq %ld\n", freq);
#endif

    dsd2pcm->type = type;
    dsd2pcm->channels = channels;
    dsd2pcm->dsd_samplerate = dsd_samplerate;
    dsd2pcm->pcm_samplerate = pcm_samplerate;

    dsd2pcm->work = rk_dsp_work_create(RKDSP_CODEC_WORK);
    dsd2pcm->work->id = 0;
    dsd2pcm->work->algo_type = DSP_CODEC_DSD_DECODE_OPEN;
    dsd2pcm->work->param = (uint32_t)dsd2pcm;
    dsd2pcm->work->param_size = sizeof(struct dsd2pcm);

    ret = rt_device_control(dsd2pcm->dsp, RKDSP_CTL_QUEUE_WORK, dsd2pcm->work);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(dsd2pcm->dsp, RKDSP_CTL_DEQUEUE_WORK, dsd2pcm->work);
    RT_ASSERT(ret == RT_EOK);

    return dsd2pcm;
}

int dsd2pcm_converter_convert(void *converter, uint8_t *dsd_data, uint32_t dsd_size, uint8_t *pcm_out, uint16_t bps, int dsf)
{
    struct dsd2pcm *dsd2pcm = (struct dsd2pcm *)converter;
    int ret;

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, dsd_data, dsd_size);
    rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, pcm_out, bps * sizeof(uint32_t));

    dsd2pcm->work->id = 0;
    dsd2pcm->work->algo_type = DSP_CODEC_DSD_DECODE_PROCESS;

    dsd2pcm->dsd_data = dsd_data;
    dsd2pcm->dsd_size = dsd_size;
    dsd2pcm->pcm_out = pcm_out;
    dsd2pcm->bps = 32;//bps;
    dsd2pcm->dsf = dsf;

    ret = rt_device_control(dsd2pcm->dsp, RKDSP_CTL_QUEUE_WORK, dsd2pcm->work);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(dsd2pcm->dsp, RKDSP_CTL_DEQUEUE_WORK, dsd2pcm->work);
    RT_ASSERT(ret == RT_EOK);

    ret = dsd2pcm->work->result;

    return ret;
}

void dsd2pcm_converter_deinit(void *converter)
{
    struct dsd2pcm *dsd2pcm = (struct dsd2pcm *)converter;
    int ret;

    dsd2pcm->work->id = 0;
    dsd2pcm->work->algo_type = DSP_CODEC_DSD_DECODE_CLOSE;

    ret = rt_device_control(dsd2pcm->dsp, RKDSP_CTL_QUEUE_WORK, dsd2pcm->work);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(dsd2pcm->dsp, RKDSP_CTL_DEQUEUE_WORK, dsd2pcm->work);
    RT_ASSERT(ret == RT_EOK);

    ret = dsd2pcm->work->result;
    rk_dsp_work_destroy(dsd2pcm->work);
    rt_device_close(dsd2pcm->dsp);

    rkdsp_free(dsd2pcm);
}
#endif

