/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

#ifdef AUDIO_DECODER_DIFF
#include "play_diff.h"
#include "dsd_common.h"
#include "diff_dec.h"

struct play_diff
{
    struct diff diff;
    bool has_post;
    uint32_t start_time;
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;
    void *userdata;
};

typedef struct play_diff *play_diff_handle_t;

int play_diff_check_impl(char *buf, int len)
{
    if (!chunk_id_is(buf, "FRM8"))
        return RK_AUDIO_FAILURE;

    if (!chunk_id_is(buf + 12, "DSD "))
        return RK_AUDIO_FAILURE;

    return RK_AUDIO_SUCCESS;
}

static int _diff_read(void *self, char *buf, uint32_t size)
{
    play_diff_handle_t _diff = (play_diff_handle_t)self;

    return _diff->input(_diff->userdata, buf, size);
}

static int _diff_write(void *self, char *buf, uint32_t size)
{
    play_diff_handle_t _diff = (play_diff_handle_t)self;

    return _diff->output(_diff->userdata, buf, size);
}

int play_diff_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg)
{
    RK_AUDIO_LOG_D("play_diff_init_impl in");
    play_diff_handle_t diff = (play_diff_handle_t) audio_calloc(1, sizeof(*diff));
    if (!diff)
        return RK_AUDIO_FAILURE;
    memset(diff, 0, sizeof(*diff));
    diff->has_post = false;
    diff->input = cfg->input;
    diff->output = cfg->output;
    diff->post = cfg->post;
    diff->userdata = cfg->userdata;
    self->userdata = (void *) diff;

    RK_AUDIO_LOG_D("play_decoder type [%s]", self->type);

    diff->diff.read = _diff_read;
    diff->diff.write = _diff_write;
    if (diff_open(&diff->diff) != RK_AUDIO_SUCCESS)
        return RK_AUDIO_FAILURE;

    diff->post(diff->userdata, diff->diff.out_rate, 32, diff->diff.out_ch);
    diff->has_post = true;

    if (cfg->start_time)
        diff->start_time = cfg->start_time;

    return RK_AUDIO_SUCCESS;
}

play_decoder_error_t play_diff_process_impl(struct play_decoder *self)
{
    play_diff_handle_t diff = (play_diff_handle_t) self->userdata;
    RK_AUDIO_LOG_D("play_diff_process");

    return diff_process(&diff->diff);
}

bool play_diff_get_post_state_impl(struct play_decoder *self)
{
    play_diff_handle_t diff = (play_diff_handle_t) self->userdata;
    return diff->has_post;
}

void play_diff_destroy_impl(struct play_decoder *self)
{
    RK_AUDIO_LOG_D("in");
    play_diff_handle_t diff = (play_diff_handle_t) self->userdata;
    if (diff)
    {
        RK_AUDIO_LOG_D("free diffdec buffer.");
        audio_free(diff);
    }

    RK_AUDIO_LOG_D("out");
}
#endif

