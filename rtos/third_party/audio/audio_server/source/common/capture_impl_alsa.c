/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2021 Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

#ifdef OS_IS_LINUX
int capture_device_open_impl(struct capture_device *self, capture_device_cfg_t *cfg)
{
    return RK_AUDIO_SUCCESS;
}

int capture_device_start_impl(struct capture_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int capture_set_volume(int vol, int vol2)
{
    int _vol, _vol2;
    return ((_vol << 16) | _vol2 & 0xFFFF);
}

int capture_device_read_impl(struct capture_device *self, const char *data, size_t data_len)
{
    return data_len;
}

int capture_device_stop_impl(struct capture_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int capture_device_abort_impl(struct capture_device *self)
{
    return RK_AUDIO_SUCCESS;
}

int capture_device_close_impl(struct capture_device *self)
{
    return RK_AUDIO_SUCCESS;
}
#endif
