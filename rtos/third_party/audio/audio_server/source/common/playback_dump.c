/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */
#include <stdio.h>
#include "AudioConfig.h"
#include "audio_pcm.h"

struct dump_data
{
    int fd;
    int fakesink;
    struct wav_header head;
    int samples;
    int bytes_per_sample;
};

int playback_dump_open_impl(struct playback_device *self, playback_device_cfg_t *cfg)
{
    struct dump_data *ddata = audio_malloc(sizeof(struct dump_data));

    if (!ddata)
        return RK_AUDIO_FAILURE;

    memset(ddata, 0, sizeof(struct dump_data));
    self->userdata = ddata;
    if (strncmp((char *)cfg->card_name, "fakesink", 8) == 0)
    {
        ddata->fakesink = 1;
        return RK_AUDIO_SUCCESS;
    }

    ddata->fd = audio_fopen((char *)cfg->card_name, "wb+");
    if (!ddata->fd)
    {
        audio_free(ddata);
        return RK_AUDIO_FAILURE;
    }

    ddata->bytes_per_sample = (cfg->bits >> 3) * cfg->channels;

    wav_header_init(&ddata->head, cfg->samplerate, cfg->bits, cfg->channels);
    RK_AUDIO_LOG_V("Open %s success. rata %d ch %d bits %d",
                   cfg->card_name, cfg->samplerate,
                   cfg->bits, cfg->channels);

    audio_fseek(ddata->fd, 44, SEEK_SET);

    return RK_AUDIO_SUCCESS;
}

int playback_dump_start_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int playback_dump_write_impl(struct playback_device *self, const char *data, size_t data_len)
{
    struct dump_data *ddata = (struct dump_data *)self->userdata;
    int write_err;

    if (ddata->fakesink)
        return data_len;

    if (!ddata->fd)
    {
        RK_AUDIO_LOG_E("Should call open first");
        return RK_AUDIO_FAILURE;
    }

    write_err = audio_fwrite((void *)data, 1, data_len, ddata->fd);
    if (write_err < 0)
        RK_AUDIO_LOG_W("write_frames: %d", write_err);
    else
        ddata->samples += data_len / ddata->bytes_per_sample;

    return data_len;
}

int playback_dump_stop_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int playback_dump_abort_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

void playback_dump_close_impl(struct playback_device *self)
{
    struct dump_data *ddata = (struct dump_data *)self->userdata;
    RK_AUDIO_LOG_V("close");

    if (ddata && ddata->fd && !ddata->fakesink)
    {
        wav_header_complete(&ddata->head, ddata->samples);
        audio_fseek(ddata->fd, 0, SEEK_SET);
        audio_fwrite((void *)&ddata->head, 1, sizeof(ddata->head), ddata->fd);
        audio_fclose(ddata->fd);
        audio_free(ddata);
    }
}

