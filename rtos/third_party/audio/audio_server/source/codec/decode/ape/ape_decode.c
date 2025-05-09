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

#include "ape_decode.h"

#ifdef AUDIO_DECODER_APE

#if 0//def __RT_THREAD__

#define DSP_CODEC_APE_DECODE_OPEN       0x5000000C
#define DSP_CODEC_APE_DECODE_PROCESS    0x5000000D
#define DSP_CODEC_APE_DECODE_CLOSE      0x5000000E

#include "drv_dsp.h"
static struct rt_device *g_dsp_handle;
static struct dsp_work *g_dsp_work;
extern void *get_dsp_fw_by_id(int id);
int32_t ape_dec_init(APEDec **dec)
{
    int ret;

    g_dsp_handle = rt_device_find("dsp0");
    rt_device_control(g_dsp_handle, RKDSP_CTL_PREPARE_IMAGE_DATA, (void *)get_dsp_fw_by_id(0));
    ret = rt_device_open(g_dsp_handle, RT_DEVICE_OFLAG_RDWR);
    RT_ASSERT(ret == RT_EOK);

#ifdef RT_USING_PM_DVFS
    uint32_t freq;
    ret = rt_device_control(g_dsp_handle, RKDSP_CTL_SET_FREQ, (void *)297000000);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(g_dsp_handle, RKDSP_CTL_GET_FREQ, (void *)&freq);
    RT_ASSERT(ret == RT_EOK);
    RK_AUDIO_LOG_V("DSP set freq %ld\n", freq);
#endif

    g_dsp_work = rk_dsp_work_create(RKDSP_CODEC_WORK);
    g_dsp_work->id = 0;
    g_dsp_work->algo_type = DSP_CODEC_APE_DECODE_OPEN;
    ret = rt_device_control(g_dsp_handle, RKDSP_CTL_QUEUE_WORK, g_dsp_work);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(g_dsp_handle, RKDSP_CTL_DEQUEUE_WORK, g_dsp_work);
    RT_ASSERT(ret == RT_EOK);

    *dec = (APEDec *)g_dsp_work->param;

    return g_dsp_work->result;
}

int32_t ape_dec_process(APEDec *dec)
{
    int ret;

    rt_hw_cpu_dcache_ops(RT_HW_CACHE_FLUSH, dec->in.buf.buf_in, dec->in.buf.buf_in_size);

    g_dsp_work->id = 0;
    g_dsp_work->algo_type = DSP_CODEC_APE_DECODE_PROCESS;
    ret = rt_device_control(g_dsp_handle, RKDSP_CTL_QUEUE_WORK, g_dsp_work);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(g_dsp_handle, RKDSP_CTL_DEQUEUE_WORK, g_dsp_work);
    RT_ASSERT(ret == RT_EOK);

    if (dec->out_len > 0)
        rt_hw_cpu_dcache_ops(RT_HW_CACHE_INVALIDATE, dec->out_buf, dec->out_len * 2 * sizeof(short));

    return g_dsp_work->result;
}

int32_t ape_dec_deinit(APEDec *dec)
{
    int ret;

    g_dsp_work->id = 0;
    g_dsp_work->algo_type = DSP_CODEC_APE_DECODE_CLOSE;
    ret = rt_device_control(g_dsp_handle, RKDSP_CTL_QUEUE_WORK, g_dsp_work);
    RT_ASSERT(ret == RT_EOK);
    ret = rt_device_control(g_dsp_handle, RKDSP_CTL_DEQUEUE_WORK, g_dsp_work);
    RT_ASSERT(ret == RT_EOK);
    rk_dsp_work_destroy(g_dsp_work);
    rt_device_close(g_dsp_handle);

    return g_dsp_work->result;
}

#else

int32_t ape_dec_init(APEDec **dec)
{
    APEDec *decoder;

    init_ape(&decoder);
    decoder->out_buf = (uint8_t *)audio_malloc(BLOCKS_PER_LOOP * 2 * sizeof(short));

    *dec = decoder;

    return RK_AUDIO_SUCCESS;
}
int32_t ape_dec_process(APEDec *dec)
{
    ape_frame_prepare(dec);
    ape_decode(dec);

    /* Decode finish check */
    if (dec->apeobj->TimePos == dec->apeobj->total_blocks)
        return -2;
    if (dec->apeobj->TimePos > dec->apeobj->total_blocks)
        return -1;

    return RK_AUDIO_SUCCESS;
}
int32_t ape_dec_deinit(APEDec *dec)
{
    audio_free(dec->out_buf);
    ape_free(dec);

    return RK_AUDIO_SUCCESS;
}

#endif  // __RT_THREAD__
#endif  // RT_USING_EXT_FLAC_DECODER
