/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2020 Fuzhou Rockchip Electronics Co., Ltd
 */

#ifndef PLAY_AMR_H
#define PLAY_AMR_H

#ifdef __cplusplus
extern "C" {
#endif

#include "AudioConfig.h"

int play_amr_check_impl(char *buf, int len);
int play_amr_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg);
play_decoder_error_t play_amr_process_impl(struct play_decoder *self);
bool play_amr_get_post_state_impl(struct play_decoder *self);
void play_amr_destroy_impl(struct play_decoder *self);

#ifdef  CONFIG_FWANALYSIS_SEGMENT
#define DEFAULT_AMR_DECODER { \
    .type = "amr", \
    .segment = SEGMENT_ID_PLAY_AMR, \
    .check = play_amr_check_impl, \
    .init = play_amr_init_impl, \
    .process = play_amr_process_impl, \
    .get_post_state = play_amr_get_post_state_impl, \
    .destroy = play_amr_destroy_impl, \
}
#else
#define DEFAULT_AMR_DECODER { \
    .type = "amr", \
    .check = play_amr_check_impl, \
    .init = play_amr_init_impl, \
    .process = play_amr_process_impl, \
    .get_post_state = play_amr_get_post_state_impl, \
    .destroy = play_amr_destroy_impl, \
}
#endif

#ifdef __cplusplus
}
#endif

#endif
