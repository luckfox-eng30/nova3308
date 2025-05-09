/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"
#include "audio_pcm.h"

struct
{
    union
    {
        struct {
            int vol;    /* playback:L/R gain, capture: unused*/
        }play;
        struct {
            int dB;     /* playback:unused, capture: L/R gain*/
            int dB_lp;  /* playback:unused, capture: loopback gain*/
        }rec;
    };
    bool changed;
} play_vol, rec_vol;

struct pcm
{
    void *device;
    int type;
    bool prepared;
    struct pcm_config config;
    void *user_data;
};

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))
#endif

struct pcm *pcm_open(const int dev_id, int flag)
{
    struct pcm *pcm_dev = audio_malloc(sizeof(struct pcm));
    if (pcm_dev == NULL)
    {
        RK_AUDIO_LOG_E("pcm malloc failed");
        return NULL;
    }
    void *audio_dev = audio_device_open(dev_id, flag);
    if (audio_dev == NULL)
    {
        audio_free(pcm_dev);
        return NULL;
    }

    switch (flag)
    {
    case AUDIO_FLAG_RDONLY:
        pcm_dev->type = PCM_IN;
        break;

    case AUDIO_FLAG_WRONLY:
        pcm_dev->type = PCM_OUT;
        break;

    default:
        RK_AUDIO_LOG_E("unsupport flag:%d", flag);
        pcm_dev->type = PCM_OUT;
        break;
    }
    pcm_dev->device = audio_dev;
    pcm_dev->prepared = RK_AUDIO_FALSE;
    RK_AUDIO_LOG_V("%s:%p", __func__, audio_dev);

    return pcm_dev;
}

static unsigned long m_bytes_to_frames(struct pcm *pcm_dev, unsigned long bytes)
{
    return bytes / (pcm_dev->config.channels * (pcm_dev->config.bits >> 3));
}

int pcm_set_config(struct pcm *pcm_dev, struct pcm_config config)
{
    struct AUDIO_PARAMS aparams;
    int ret;

    if (!pcm_dev)
        return RK_AUDIO_FAILURE;

    pcm_dev->config = config;
    struct audio_buf *abuf = audio_malloc(sizeof(struct audio_buf));
    if (!abuf)
    {
        RK_AUDIO_LOG_E("malloc failed %d", sizeof(struct audio_buf));
        return RK_AUDIO_FAILURE;
    }
    pcm_dev->user_data = abuf;

    abuf->buf = (uint8_t *)audio_malloc_uncache(config.period_size * config.period_count);
    if (!abuf->buf)
    {
        RK_AUDIO_LOG_E("unchache malloc failed %d", config.period_size * config.period_count);
        return RK_AUDIO_FAILURE;
    }

    memset(abuf->buf, 0x00, config.period_size * config.period_count);
    abuf->buf_size = m_bytes_to_frames(pcm_dev, config.period_size * config.period_count);
    abuf->period_size = m_bytes_to_frames(pcm_dev, config.period_size);
    aparams.channels = config.channels;
    aparams.sampleRate = config.rate;
    aparams.sampleBits = config.bits;
    ret = audio_device_control(pcm_dev->device, RK_AUDIO_CTL_PCM_PREPARE, abuf);
    if (ret)
    {
        audio_free_uncache(abuf->buf);
        audio_free(abuf);
        return ret;
    }
    /* If audio plugin is disable, the result value will always be ERROR,
     * do not check result value here
     **/
    audio_device_control(pcm_dev->device, RK_AUDIO_CTL_PLUGIN_PREPARE, (void *)SND_PCM_TYPE_SOFTVOL);
    ret = audio_device_control(pcm_dev->device, RK_AUDIO_CTL_HW_PARAMS, &aparams);
    if (ret)
    {
        audio_free_uncache(abuf->buf);
        audio_free(abuf);
        return ret;
    }
    pcm_dev->prepared = RK_AUDIO_TRUE;
    if (/*rec_vol.changed && */pcm_dev->type == PCM_IN)
    {
        audio_device_set_gain(pcm_dev->device, RECORD_CARD_CHANNEL_0, rec_vol.rec.dB);
        audio_device_set_gain(pcm_dev->device, RECORD_CARD_CHANNEL_1, rec_vol.rec.dB);
        rec_vol.rec.dB = audio_device_get_gain(pcm_dev->device, RECORD_CARD_CHANNEL_0);

        audio_device_set_gain(pcm_dev->device, RECORD_CARD_CHANNEL_2, rec_vol.rec.dB_lp);
        rec_vol.rec.dB_lp = audio_device_get_gain(pcm_dev->device, RECORD_CARD_CHANNEL_2);

        rec_vol.changed = RK_AUDIO_FALSE;
    }
    if (/*play_vol.changed && */pcm_dev->type == PCM_OUT)
    {
        audio_device_set_vol(pcm_dev->device, play_vol.play.vol);
        play_vol.changed = RK_AUDIO_FALSE;
    }

    return 0;
}

int pcm_set_volume(struct pcm *pcm_dev, int vol, int vol2, int flag)
{
    if (!pcm_dev || !pcm_dev->prepared)
    {
        switch (flag)
        {
        case AUDIO_FLAG_RDONLY:
            rec_vol.rec.dB = vol;
            rec_vol.rec.dB_lp = vol2;
            rec_vol.changed = RK_AUDIO_TRUE;
            return RK_AUDIO_SUCCESS;

        case AUDIO_FLAG_WRONLY:
            play_vol.play.vol = vol;
            play_vol.changed = RK_AUDIO_TRUE;
            return RK_AUDIO_SUCCESS;

        default:
            RK_AUDIO_LOG_E("unsupport flag:%d", flag);
            return RK_AUDIO_FAILURE;
        }
    }
    switch (flag)
    {
    case AUDIO_FLAG_RDONLY:
        rec_vol.rec.dB = vol;
        rec_vol.rec.dB_lp = vol2;
        audio_device_set_gain(pcm_dev->device, RECORD_CARD_CHANNEL_0, vol);
        audio_device_set_gain(pcm_dev->device, RECORD_CARD_CHANNEL_1, vol);
        audio_device_set_gain(pcm_dev->device, RECORD_CARD_CHANNEL_2, vol2);
        return RK_AUDIO_SUCCESS;

    case AUDIO_FLAG_WRONLY:
        play_vol.play.vol = vol;
        audio_device_set_vol(pcm_dev->device, vol);
        return RK_AUDIO_SUCCESS;

    default:
        RK_AUDIO_LOG_E("unsupport flag:%d", flag);
        return RK_AUDIO_FAILURE;
    }
}

int pcm_get_volume(struct pcm *pcm_dev, int *vol, int *vol2, int flag)
{
    int ret;

    if (pcm_dev && pcm_dev->prepared)
    {
        switch (flag)
        {
        case AUDIO_FLAG_RDONLY:
            rec_vol.rec.dB = audio_device_get_gain(pcm_dev->device, RECORD_CARD_CHANNEL_0);
            rec_vol.rec.dB_lp = audio_device_get_gain(pcm_dev->device, RECORD_CARD_CHANNEL_2);
            break;

        case AUDIO_FLAG_WRONLY:
            ret = audio_device_get_vol(pcm_dev->device);
            if (ret != RK_AUDIO_FAILURE)
                play_vol.play.vol = ret;
            break;

        default:
            RK_AUDIO_LOG_E("unsupport flag:%d", flag);
            return RK_AUDIO_FAILURE;
        }
    }

    switch (flag)
    {
    case AUDIO_FLAG_RDONLY:
        if (vol)
            *vol = rec_vol.rec.dB;
        if (vol2)
            *vol2 = rec_vol.rec.dB_lp;
        return RK_AUDIO_SUCCESS;

    case AUDIO_FLAG_WRONLY:
        if (vol)
            *vol = play_vol.play.vol;
        if (vol2)
            *vol2 = play_vol.play.vol;
        return RK_AUDIO_SUCCESS;

    default:
        RK_AUDIO_LOG_E("unsupport flag:%d", flag);
        return RK_AUDIO_FAILURE;
    }
}

int pcm_prepare(struct pcm *pcm_dev)
{
    return 0;
}

int pcm_start(struct pcm *pcm_dev)
{
    return 0;//audio_device_control(pcm_dev->device, RK_AUDIO_CTL_START, NULL);
}

int pcm_stop(struct pcm *pcm_dev)
{
    if (!pcm_dev)
        return RK_AUDIO_FAILURE;

    audio_device_control(pcm_dev->device, RK_AUDIO_CTL_PLUGIN_RELEASE, (void *)SND_PCM_TYPE_SOFTVOL);
    audio_device_control(pcm_dev->device, RK_AUDIO_CTL_STOP, NULL);
    audio_device_control(pcm_dev->device, RK_AUDIO_CTL_PCM_RELEASE, NULL);
    if (pcm_dev->user_data)
    {
        if (((struct audio_buf *)pcm_dev->user_data)->buf)
            audio_free_uncache(((struct audio_buf *)pcm_dev->user_data)->buf);
        audio_free(pcm_dev->user_data);
    }
    pcm_dev->prepared = RK_AUDIO_FALSE;

    return 0;
}

int pcm_close(struct pcm *pcm_dev)
{
    if (!pcm_dev)
        return RK_AUDIO_FAILURE;
    audio_device_close(pcm_dev->device);
    audio_free(pcm_dev);

    return 0;
}

unsigned long pcm_write(struct pcm *pcm_dev, void *data, unsigned long bytes)
{
    if (!pcm_dev)
        return RK_AUDIO_FAILURE;
    if (pcm_dev->type & PCM_IN)
        return PCM_WRONG_TYPE;
    if (bytes != pcm_dev->config.period_size)
        return PCM_WRONG_LENGTH;
    unsigned long frames = m_bytes_to_frames(pcm_dev, bytes);
    return audio_device_write(pcm_dev->device, data, frames);
}

unsigned long pcm_read(struct pcm *pcm_dev, void *data, unsigned long bytes)
{
    if (!pcm_dev)
        return RK_AUDIO_FAILURE;
    if (!(pcm_dev->type & PCM_IN))
        return PCM_WRONG_TYPE;
    if (bytes != pcm_dev->config.period_size)
        return PCM_WRONG_LENGTH;
    unsigned long frames = m_bytes_to_frames(pcm_dev, bytes);
    return audio_device_read(pcm_dev->device, data, frames);
}
