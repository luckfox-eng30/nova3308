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

#include "AudioConfig.h"
#include "play_ape.h"
#include "ape_tag.h"

#ifdef AUDIO_DECODER_APE

static int play_ape_fill_buffer(void *arg);

int play_ape_check_impl(char *buf, int len)
{
    int id3_len;

    id3_len = check_ID3V2_tag(buf);
    if (id3_len)
    {
        if (id3_len + 10 + 2 > len)
        {
            RK_AUDIO_LOG_W("buf not enough, abort");
            return RK_AUDIO_FAILURE;
        }
        buf += (id3_len + 10);
    }

    if ((buf[0] == 'M') &&
        (buf[1] == 'A') &&
        (buf[2] == 'C') &&
        (buf[3] == ' '))
    {
        return RT_EOK;
    }

    return RT_ERROR;
}

int play_ape_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg)
{
    struct play_ape *ape = (struct play_ape *)rt_calloc(1, sizeof(struct play_ape));
    if (!ape)
        return -RT_ERROR;
    ape->has_post = 0;
    ape->input = cfg->input;
    ape->output = cfg->output;
    ape->post = cfg->post;
    ape->userdata = cfg->userdata;
    self->userdata = (void *)ape;

    ape_dec_init(&ape->dec);

    ape->dec->in.buf.buf_in_size = APE_BUFFER_IN;
    ape->dec->in.buf.buf_in = rkdsp_malloc(ape->dec->in.buf.buf_in_size);
    memset(ape->dec->in.buf.buf_in, 0, ape->dec->in.buf.buf_in_size);
    ape->dec->apeobj->file_size = ape->dec->in.buf.file_size = player_get_file_length(ape->userdata);
    ape->dec->in.cb = play_ape_fill_buffer;
    ape->dec->in.cb_param = ape;

    RK_AUDIO_LOG_V("init success");
    ape->dec->in.buf.buf_in_left = 0;

    return RT_EOK;
}

int play_ape_post(struct play_ape *ape)
{
    APEContext *apec = ape->dec->apeobj;
    ape->post(ape->userdata,  apec->samplerate, apec->bps, apec->channels);
    RK_AUDIO_LOG_V("Post %ld %d %d", apec->samplerate, apec->bps, apec->channels);
    ape->has_post = 1;

    return RT_EOK;
}

static int play_ape_fill_buffer(void *arg)
{
    struct play_ape *ape = (struct play_ape *)arg;
    APEDec *dec = ape->dec;
    uint32_t read_bytes;
    if (dec->in.buf.buf_in_left)
    {
        if (dec->in.buf.buf_in_left > (APE_BUFFER_IN / 2))
        {
            RK_AUDIO_LOG_E("error left %ld", dec->in.buf.buf_in_left);
            return -1;
        }
        memcpy(dec->in.buf.buf_in, dec->in.buf.buf_in + APE_BUFFER_IN / 2 - dec->in.buf.buf_in_left, APE_BUFFER_IN / 2 + dec->in.buf.buf_in_left);
        read_bytes = ape->input(ape->userdata, (char *)(dec->in.buf.buf_in + APE_BUFFER_IN / 2 + dec->in.buf.buf_in_left),
                                APE_BUFFER_IN / 2 - dec->in.buf.buf_in_left);
        if (read_bytes <= 0)
            return read_bytes;
//            read_bytes = dec->in.buf.buf_in_left;

        read_bytes += dec->in.buf.buf_in_left;
    }
    else
    {
        memcpy(dec->in.buf.buf_in, dec->in.buf.buf_in + APE_BUFFER_IN / 2, APE_BUFFER_IN / 2);
        read_bytes = ape->input(ape->userdata, (char *)dec->in.buf.buf_in + APE_BUFFER_IN / 2,
                                APE_BUFFER_IN / 2);
        if (read_bytes <= 0)
            return read_bytes;
    }
    dec->in.buf.buf_in_size = APE_BUFFER_IN / 2 + read_bytes;
    dec->in.buf.buf_in_left = read_bytes;
    dec->in.read_bytes = 0;

    return read_bytes;
}

play_decoder_error_t play_ape_process_impl(struct play_decoder *self)
{
    struct play_ape *ape = (struct play_ape *) self->userdata;
    APEDec *dec = ape->dec;
    int read_bytes;
    uint32_t write_bytes;
    int finish = 0;
    char buf[10];
    int ret;
    int err = PLAY_DECODER_SUCCESS;

    dec->ID3_len = 0;
    read_bytes = ape->input(ape->userdata, buf, 10);
    if (read_bytes <= 0)
        return PLAY_DECODER_INPUT_ERROR;
    dec->ID3_len = check_ID3V2_tag(buf);
    if (dec->ID3_len == 0)
    {
        player_preprocess_seek(ape->userdata, 0);
    }
    else
    {
        dec->ID3_len += 10;
        player_preprocess_seek(ape->userdata, dec->ID3_len);
        ape->dec->in.buf.file_tell = dec->ID3_len;
    }

    ret = ape_read_header(dec, ape);
    if (ret < 0)
    {
        RK_AUDIO_LOG_E("ape_read_header %d\n", ret);
        return PLAY_DECODER_DECODE_ERROR;
    }
    play_ape_post(ape);

    while (finish == 0)
    {
        read_bytes = play_ape_fill_buffer(ape);
        if (read_bytes == -1)
        {
            err = PLAY_DECODER_INPUT_ERROR;
            break;
        }
        if (read_bytes == 0)
        {
            err = PLAY_DECODER_SUCCESS;
            break;
        }

        dec->out_len = 0;
        ret = ape_dec_process(dec);
        if (ret < 0)
        {
            if (ret == -2)
            {
                err = PLAY_DECODER_SUCCESS;
                finish = 1;
            }
            else
            {
                err = PLAY_DECODER_DECODE_ERROR;
                break;
            }
        }

        if (dec->out_len > 0)
        {
            write_bytes = ape->output(ape->userdata,
                                      (char *)dec->out_buf,
                                      dec->out_len * 2 * sizeof(short));
            if (write_bytes < 0)
            {
                err = PLAY_DECODER_OUTPUT_ERROR;
                break;
            }
        }
    }
    ape_release_header(dec);

    return err;
}

bool play_ape_get_post_state_impl(struct play_decoder *self)
{
    struct play_ape *ape = (struct play_ape *) self->userdata;
    return ape->has_post;
}

void play_ape_destroy_impl(struct play_decoder *self)
{
    struct play_ape *ape = (struct play_ape *) self->userdata;

    rkdsp_free(ape->dec->in.buf.buf_in);
    ape_dec_deinit(ape->dec);
    rt_free(ape);
}

#endif  // AUDIO_DECODER_APE
