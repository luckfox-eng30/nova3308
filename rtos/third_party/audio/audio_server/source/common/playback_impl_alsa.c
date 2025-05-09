/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

#ifdef OS_IS_LINUX
int playback_device_open_impl(struct playback_device *self, playback_device_cfg_t *cfg)
{
    return RK_AUDIO_SUCCESS;
}

int playback_device_start_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int playback_set_volume(int vol)
{
    int _vol;
    return _vol;
}

int playback_device_write_impl(struct playback_device *self, const char *data, size_t data_len)
{
    return data_len;
}

int playback_device_stop_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int playback_device_abort_impl(struct playback_device *self)
{
    return RK_AUDIO_SUCCESS;
}

void playback_device_close_impl(struct playback_device *self)
{
}
#endif
