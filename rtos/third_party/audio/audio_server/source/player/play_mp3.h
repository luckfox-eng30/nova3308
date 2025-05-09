/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#ifndef PLAY_MP3_H
#define PLAY_MP3_H
#ifdef __cplusplus
extern "C" {
#endif

#include "AudioConfig.h"

#include "mp3_hal.h"
#include "mp3dec.h"

#ifndef MP3_DECODE_FRAME_COUNT
#define MP3_DECODE_FRAME_COUNT 2
#endif

#ifdef  CONFIG_FWANALYSIS_SEGMENT
#define DEFAULT_MP3_DECODER { \
        .type = "mp3", \
        .support_seek = 1, \
        .segment = SEGMENT_ID_PLAY_MP3, \
        .check = play_mp3_check_impl, \
        .init = play_mp3_init_impl, \
        .process = play_mp3_process_impl, \
        .get_post_state = play_mp3_get_post_state_impl, \
        .destroy = play_mp3_destroy_impl, \
    }
#else
#define DEFAULT_MP3_DECODER { \
        .type = "mp3", \
        .support_seek = 1, \
        .check = play_mp3_check_impl, \
        .init = play_mp3_init_impl, \
        .process = play_mp3_process_impl, \
        .get_post_state = play_mp3_get_post_state_impl, \
        .destroy = play_mp3_destroy_impl, \
    }
#endif

struct play_mp3
{
    bool has_post;
    char read_buf[MAINBUF_SIZE * MP3_DECODE_FRAME_COUNT];
    uint32_t start_time;
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;
    void *userdata;
};

int play_mp3_check_impl(char *buf, int len);
int play_mp3_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg);
play_decoder_error_t play_mp3_process_impl(struct play_decoder *self);
bool play_mp3_get_post_state_impl(struct play_decoder *self);
void play_mp3_destroy_impl(struct play_decoder *self);

#ifdef __cplusplus
}
#endif
#endif
