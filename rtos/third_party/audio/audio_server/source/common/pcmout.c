/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2020 Fuzhou Rockchip Electronics Co., Ltd
 */
#include <stdio.h>
#include "AudioConfig.h"
#include "audio_pcm.h"

static FILE *fd = NULL;

int pcmout_open_impl(struct playback_device *self, playback_device_cfg_t *cfg)
{
    if (fd == NULL)
        fd = fopen(cfg->card_name, "w+");

    RK_AUDIO_LOG_V("%s %p", cfg->card_name, fd);
    if (fd)
        return RK_AUDIO_SUCCESS;
    else
        return RK_AUDIO_FAILURE;
}

int pcmout_start_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int pcmout_write_impl(struct playback_device *self, const char *data, size_t data_len)
{
    RK_AUDIO_LOG_D("%p %d", data, data_len);
    if (fd)
        fwrite(data, 1, data_len, fd);

    return data_len;
}

int pcmout_stop_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int pcmout_abort_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

void pcmout_close_impl(struct playback_device *self)
{
    if (fd)
    {
        RK_AUDIO_LOG_V("close");
        fclose(fd);
        fd = NULL;
    }
}
