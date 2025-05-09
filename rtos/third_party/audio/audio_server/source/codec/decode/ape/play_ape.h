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

#ifndef PLAY_APE_H
#define PLAY_APE_H

#ifdef __cplusplus
extern "C" {
#endif

#include "ape_decode.h"
#include "ape.h"

#define DEFAULT_APE_DECODER { \
        .type = "ape", \
        .support_seek = 0, \
        .check = play_ape_check_impl, \
        .init = play_ape_init_impl, \
        .process = play_ape_process_impl, \
        .get_post_state = play_ape_get_post_state_impl, \
        .destroy = play_ape_destroy_impl, \
    }

struct play_ape
{
    bool has_post;
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;

    APEDec *dec;

    void *userdata;
};

int play_ape_check_impl(char *buf, int len);
int play_ape_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg);
play_decoder_error_t play_ape_process_impl(struct play_decoder *self);
bool play_ape_get_post_state_impl(struct play_decoder *self);
void play_ape_destroy_impl(struct play_decoder *self);

#ifdef __cplusplus
}
#endif
#endif
