/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#ifndef PLAY_DSF_H
#define PLAY_DSF_H
#ifdef __cplusplus
extern "C" {
#endif

#include "AudioConfig.h"

#define DEFAULT_DSF_DECODER { \
        .type = "dsf", \
        .support_seek = 0, \
        .check = play_dsf_check_impl, \
        .init = play_dsf_init_impl, \
        .process = play_dsf_process_impl, \
        .get_post_state = play_dsf_get_post_state_impl, \
        .destroy = play_dsf_destroy_impl, \
    }

int play_dsf_check_impl(char *buf, int len);
int play_dsf_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg);
play_decoder_error_t play_dsf_process_impl(struct play_decoder *self);
bool play_dsf_get_post_state_impl(struct play_decoder *self);
void play_dsf_destroy_impl(struct play_decoder *self);

#ifdef __cplusplus
}
#endif
#endif
