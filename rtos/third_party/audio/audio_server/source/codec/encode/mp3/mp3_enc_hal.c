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

void *AudioMp3EncodeOpen(struct audio_config *mp3_cfg)
{
    mp3_enc_t *mp3_enc = audio_malloc(sizeof(mp3_enc_t));
    if (!mp3_enc)
        return NULL;

    mp3_enc->enc = Mp3EncodeVariableInit(mp3_cfg->sample_rate, mp3_cfg->channels, SAMPLE_PER_FRAME);
    mp3_enc->frame_size = mp3_enc->enc->frame_size;
    if (mp3_enc->enc->frame_size <= 0)
    {
        RK_AUDIO_LOG_E("MP3 encoder init failed! r:%d c:%d\n", mp3_cfg->sample_rate, mp3_cfg->channels);
        return NULL;
    }
    RK_AUDIO_LOG_V("MP3 encoder init %pr:%d c:%d s:%d\n", mp3_enc,
            mp3_cfg->sample_rate, mp3_cfg->channels,
            mp3_enc->enc->frame_size);

    mp3_enc->buffer_in = (short *)mp3_enc->enc->config.in_buf;

    return mp3_enc;
}

int32_t AudioMp3Encode(mp3_enc_t *mp3_enc)
{
    return L3_compress(mp3_enc->enc, 0, &mp3_enc->buffer_out);
}

int32_t AudioMp3EncodeClose(mp3_enc_t *mp3_enc)
{
    RK_AUDIO_LOG_V("mp3 encode close, %p %p", mp3_enc, mp3_enc->buffer_out);
    audio_free(mp3_enc->buffer_in);
    audio_free(mp3_enc);
    return 0;
}
#endif
