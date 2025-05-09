/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

#ifdef AUDIO_DECODER_DSF
#include "play_dsf.h"
#include "dsd_common.h"
#include "dsf_dec.h"

struct play_dsf
{
    struct dsf dsf;
    bool has_post;
    uint32_t start_time;
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;
    void *userdata;
};

typedef struct play_dsf *play_dsf_handle_t;

int play_dsf_check_impl(char *buf, int len)
{
    if (!chunk_id_is(buf, "DSD "))
        return RK_AUDIO_FAILURE;

    return RK_AUDIO_SUCCESS;
}

static int _dsf_read(void *self, char *buf, uint32_t size)
{
    play_dsf_handle_t _dsf = (play_dsf_handle_t)self;

    return _dsf->input(_dsf->userdata, buf, size);
}

static int _dsf_write(void *self, char *buf, uint32_t size)
{
    play_dsf_handle_t _dsf = (play_dsf_handle_t)self;

    return _dsf->output(_dsf->userdata, buf, size);
}

int play_dsf_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg)
{
    RK_AUDIO_LOG_D("play_dsf_init_impl in");
    play_dsf_handle_t dsf = (play_dsf_handle_t) audio_calloc(1, sizeof(*dsf));
    if (!dsf)
        return RK_AUDIO_FAILURE;
    memset(dsf, 0, sizeof(*dsf));
    dsf->has_post = false;
    dsf->input = cfg->input;
    dsf->output = cfg->output;
    dsf->post = cfg->post;
    dsf->userdata = cfg->userdata;
    self->userdata = (void *) dsf;

    RK_AUDIO_LOG_D("play_decoder type [%s]", self->type);

    dsf->dsf.read = _dsf_read;
    dsf->dsf.write = _dsf_write;
    if (dsf_open(&dsf->dsf) != RK_AUDIO_SUCCESS)
        return RK_AUDIO_FAILURE;

    dsf->post(dsf->userdata, dsf->dsf.out_rate, 32, dsf->dsf.out_ch);
    dsf->has_post = true;

    if (cfg->start_time)
        dsf->start_time = cfg->start_time;

    return RK_AUDIO_SUCCESS;
}

play_decoder_error_t play_dsf_process_impl(struct play_decoder *self)
{
    play_dsf_handle_t dsf = (play_dsf_handle_t) self->userdata;
    RK_AUDIO_LOG_D("play_dsf_process");

    return dsf_process(&dsf->dsf);
}

bool play_dsf_get_post_state_impl(struct play_decoder *self)
{
    play_dsf_handle_t dsf = (play_dsf_handle_t) self->userdata;
    return dsf->has_post;
}

void play_dsf_destroy_impl(struct play_decoder *self)
{
    RK_AUDIO_LOG_D("in");
    play_dsf_handle_t dsf = (play_dsf_handle_t) self->userdata;
    if (dsf)
    {
        RK_AUDIO_LOG_D("free dsfdec buffer.");
        audio_free(dsf);
    }

    RK_AUDIO_LOG_D("out");
}
#endif

