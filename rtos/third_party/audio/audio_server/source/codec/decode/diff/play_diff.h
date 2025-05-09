/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#ifndef PLAY_DIFF_H
#define PLAY_DIFF_H
#ifdef __cplusplus
extern "C" {
#endif

#include "AudioConfig.h"

#ifndef DIFF_DECODE_FRAME_COUNT
#define DIFF_DECODE_FRAME_COUNT 2
#endif

#define DEFAULT_DIFF_DECODER { \
        .type = "dff", \
        .support_seek = 1, \
        .check = play_diff_check_impl, \
        .init = play_diff_init_impl, \
        .process = play_diff_process_impl, \
        .get_post_state = play_diff_get_post_state_impl, \
        .destroy = play_diff_destroy_impl, \
    }

int play_diff_check_impl(char *buf, int len);
int play_diff_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg);
play_decoder_error_t play_diff_process_impl(struct play_decoder *self);
bool play_diff_get_post_state_impl(struct play_decoder *self);
void play_diff_destroy_impl(struct play_decoder *self);

#ifdef __cplusplus
}
#endif
#endif
