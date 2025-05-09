/*
 *  Copyright (C) 2019, Fuzhou Rockchip Electronics Co., Ltd.
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *  1. Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright
 *     notice, this list of conditions and the following disclaimer in the
 *     documentation and/or other materials provided with the distribution.
 *  3. Neither the name of Fuzhou Rockchip Electronics Co., Ltd. nor the
 *     names of its contributors may be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 *  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 *  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 *  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 *  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 *  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 *  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 *  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 *  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "AudioConfig.h"

#ifdef AUDIO_ENCODER_MP3

#include "mp3_enc_hal.h"
#include "record_mp3.h"

#define SAMPLE_PER_FRAME 160

struct record_mp3
{
    bool has_post;
    encode_input_t input;
    encode_output_t output;
    encode_post_t post;
    mp3_enc_t *enc;
    void *userdata;
};

int record_mp3_init(struct record_encoder *self, record_encoder_cfg_t *cfg)
{
    struct record_mp3 * mp3 = (struct record_mp3 *) audio_calloc(1, sizeof(*mp3));
    struct audio_config *mp3_cfg = (struct audio_config *)self->userdata;
    if (!mp3)
        return -1;
    RK_AUDIO_LOG_D("MP3 encode init");
    mp3->has_post = false;
    mp3->input = cfg->input;
    mp3->output = cfg->output;
    mp3->post = cfg->post;
    mp3->userdata = cfg->userdata;
    self->userdata = (void *) mp3;
    mp3->enc = AudioMp3EncodeOpen(mp3_cfg);
    RK_AUDIO_LOG_V("MP3 init SUCCESS out");

    return 0;
}

record_encoder_error_t record_mp3_process(struct record_encoder *self)
{
    struct record_mp3 * mp3 = (struct record_mp3 *) self->userdata;
    int32_t num_enc_bytes;
    RK_AUDIO_LOG_D("MP3 encode process");
    while (1)
    {
        int read_bytes = mp3->input(mp3->userdata, (char *)mp3->enc->buffer_in, mp3->enc->frame_size * 2);
        RK_AUDIO_LOG_D("record_mp3_process:%d", read_bytes);
        if (read_bytes == 0)
        {
            RK_AUDIO_LOG_V("mp3->input finish");
            return 0;
        }
        else if (read_bytes == -1)
        {
            RK_AUDIO_LOG_V("mp3->input failed");
            return RECORD_ENCODER_INPUT_ERROR;
        }
        num_enc_bytes = AudioMp3Encode(mp3->enc);
        if (num_enc_bytes < 0)
        {
            RK_AUDIO_LOG_E("MP3 encode return %ld bytes", num_enc_bytes);
            return RECORD_ENCODER_ENCODE_ERROR;
        }
        int write_bytes = mp3->output(mp3->userdata, (char *)mp3->enc->buffer_out, num_enc_bytes);
        if (write_bytes == -1)
        {
            RK_AUDIO_LOG_E("mp3->output failed");
            return RECORD_ENCODER_OUTPUT_ERROR;
        }
    }
    return 0;
}

bool record_mp3_get_post_state(struct record_encoder *self)
{
    struct record_mp3 *mp3 = (struct record_mp3 *)self->userdata;
    return mp3->has_post;
}

void record_mp3_destroy(struct record_encoder *self)
{
    struct record_mp3 *mp3 = (struct record_mp3 *)self->userdata;
    AudioMp3EncodeClose(mp3->enc);
    if (mp3)
        audio_free(mp3);
}
#endif

