/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

#ifdef AUDIO_DECODER_AMR
static int amrnb_frame_size[] = { 12, 13, 15, 17, 19, 20, 26, 31, -1, -1, -1, -1, -1, -1, -1, 0 };
static int amrwb_frame_size[] = { 17, 23, 32, 36, 40, 46, 50, 58, 60,  5, -1, -1, -1, -1, -1, 0 };

struct play_amr
{
    bool has_post;
    char read_buf[64];
    decode_input_t input;
    decode_output_t output;
    decode_post_t post;
    void *userdata;
};

typedef struct play_amr *play_amr_handle_t;
static struct amr_dec *g_Amr;
static struct audio_server_data_share g_amr_share_data;

int play_amr_check_impl(char *buf, int len)
{
    return (strncmp(buf, "#!AMR", 5));
}

int play_amr_init_impl(struct play_decoder *self, play_decoder_cfg_t *cfg)
{
    size_t read_bytes;
    char *buf;
    int type = AMR_NB;
    int ch = 1;
    int ret = RK_AUDIO_SUCCESS;
    RK_AUDIO_LOG_D("in");
    play_amr_handle_t amr = (play_amr_handle_t) audio_calloc(1, sizeof(*amr));
    if (!amr)
        return RK_AUDIO_FAILURE;
    memset(amr, 0, sizeof(*amr));
    amr->has_post = false;
    amr->input = cfg->input;
    amr->output = cfg->output;
    amr->post = cfg->post;
    amr->userdata = cfg->userdata;
    self->userdata = (void *) amr;
    buf = amr->read_buf;

    RK_AUDIO_LOG_D("play_decoder type [%s]", self->type);

    read_bytes = amr->input(amr->userdata, buf, AMR_HEADER_AMRNB_SC_LENGTH);
    if (read_bytes != AMR_HEADER_AMRNB_SC_LENGTH ||
        (memcmp(buf, AMR_HEADER_AMRNB_SC, AMR_HEADER_AMRNB_SC_LENGTH - 1) != 0))
    {
        RK_AUDIO_LOG_E("read header failed %d %s.", read_bytes, buf);
        audio_free(amr);
        return RK_AUDIO_FAILURE;
    }

    /* Check if "#!AMR\n" or "#!AMR_MC1.0\n" */
    if (buf[AMR_HEADER_AMRNB_SC_LENGTH - 1] == '_' ||
        buf[AMR_HEADER_AMRNB_SC_LENGTH - 1] == '\n')
    {
        if (buf[AMR_HEADER_AMRNB_SC_LENGTH - 1] == '_')
        {
            amr->input(amr->userdata, buf, AMR_HEADER_AMRNB_MC_LENGTH - AMR_HEADER_AMRNB_SC_LENGTH);
            amr->input(amr->userdata, buf, 4);
            ch = buf[3] & 0x0F;
        }
        type = AMR_NB;
    }
    /* Check if "#!AMR-WB\n" or "#!AMR-WB_MC1.0\n" */
    else if (buf[AMR_HEADER_AMRNB_SC_LENGTH - 1] == '-')
    {
        amr->input(amr->userdata, buf, AMR_HEADER_AMRWB_SC_LENGTH - AMR_HEADER_AMRNB_SC_LENGTH);
        if (buf[2] != '\n')
        {
            amr->input(amr->userdata, buf, AMR_HEADER_AMRWB_MC_LENGTH - AMR_HEADER_AMRWB_SC_LENGTH);
            amr->input(amr->userdata, buf, 4);
            ch = buf[3] & 0x0F;
        }
        type = AMR_WB;
    }
    /* Only support mono now */
    if (/* ch != 2 && */ch != 1)
    {
        RK_AUDIO_LOG_E("Not support %d channels", ch);
        audio_free(amr);
        return RK_AUDIO_FAILURE;
    }

    g_amr_share_data.type = TYPE_AUDIO_AMR_DEC;
    g_amr_share_data.dat[0] = (uint32_t)type;
    g_amr_share_data.dat[1] = (uint32_t)ch;
    ret = AudioDecAmrOpen((uint32_t)&g_amr_share_data);
    // ret = AudioSendMsg(TYPE_AUDIO_AMR_DEC, MEDIA_MSGBOX_CMD_DECODE_OPEN);
    if (ret < 0)
    {
        audio_free(amr);
        return RK_AUDIO_FAILURE;
    }

    g_Amr = (struct amr_dec *)g_amr_share_data.dat[0];
    if (((int32_t)g_Amr) == AMR_AB_CORE_SHARE_ADDR_INVALID)
    {
        RK_AUDIO_LOG_E("amr init Decoder error");
        play_amr_destroy_impl(self);
        return RK_AUDIO_FAILURE;
    }
    RK_AUDIO_LOG_V("amr init SUCCESS out %s%s",
                   g_Amr->is_wb ? "AMR-WB" : "AMR-NB",
                   g_Amr->is_mono ? "" : "_MC1.0");

    return RK_AUDIO_SUCCESS;
}

play_decoder_error_t play_amr_process_impl(struct play_decoder *self)
{
    play_amr_handle_t amr = (play_amr_handle_t) self->userdata;
    size_t read_bytes;
    int frame_size = 0;
    int out_frame;
    int mode, old_mode = -1;
    int ret = 0;

    amr->post(amr->userdata,
              g_Amr->is_wb ? 16000 : 8000,
              16, g_Amr->is_mono ? 1 : 2);
    RK_AUDIO_LOG_V("Post:r %d, b %d, c %d",
                   g_Amr->is_wb ? 16000 : 8000,
                   16, g_Amr->is_mono ? 1 : 2);
    amr->has_post = true;

    out_frame = g_Amr->is_wb ? SAMPLE_PER_FRAME_WB : SAMPLE_PER_FRAME;

    while (1)
    {
        // Read the mode byte
        read_bytes = amr->input(amr->userdata, (char *)g_Amr->_buffer_in, 1);
        if (read_bytes == 0)
        {
            return PLAY_DECODER_SUCCESS;
        }
        if (read_bytes < 0)
        {
            RK_AUDIO_LOG_E("Read Size %d", read_bytes);
            return PLAY_DECODER_INPUT_ERROR;
        }
        mode = ((g_Amr->_buffer_in[0] >> 3) & 0x0f);

        if (frame_size == 0)
        {
            if (mode >= sizeof(amrwb_frame_size))
            {
                RK_AUDIO_LOG_E("Mode %d out of range[0 - %d]", mode, sizeof(amrwb_frame_size) - 1);
                return PLAY_DECODER_DECODE_ERROR;
            }
            frame_size = g_Amr->is_wb ? amrwb_frame_size[mode] : amrnb_frame_size[mode];
#ifdef AMRNB_TINY
            if (frame_size <= 0 || (!g_Amr->is_wb && frame_size != 31))
#else
            if (frame_size <= 0)
#endif
            {
                RK_AUDIO_LOG_E("Not supported mode %d", mode);
                return PLAY_DECODER_DECODE_ERROR;
            }
        }

        if (old_mode != -1 && mode != old_mode)
        {
            RK_AUDIO_LOG_W("Bad frame %d != %d, %d", mode, old_mode, frame_size);
            amr->input(amr->userdata, (char *)(g_Amr->_buffer_in + 1), frame_size);
            continue;
        }
        old_mode = mode;

        // Harded code the frame size
        read_bytes = amr->input(amr->userdata, (char *)(g_Amr->_buffer_in + 1), frame_size);
        if (read_bytes != frame_size)
        {
            RK_AUDIO_LOG_E("read amr frame failed");
            return PLAY_DECODER_DECODE_ERROR;
        }
        ret = AudioDecAmrProcess((uint32_t)&g_amr_share_data);
        // ret = AudioSendMsg(TYPE_AUDIO_AMR_DEC, MEDIA_MSGBOX_CMD_DECODE);
        if (ret < 0)
        {
            return PLAY_DECODER_DECODE_ERROR;
        }
        /* PCM16LE */
        int write_bytes = amr->output(amr->userdata, (char *)g_Amr->_buffer_out, out_frame * sizeof(short));
        if (write_bytes == -1)
        {
            RK_AUDIO_LOG_E("amr_decode pcm write failed");
            return PLAY_DECODER_OUTPUT_ERROR;
        }
    }

    return RK_AUDIO_SUCCESS;
}

bool play_amr_get_post_state_impl(struct play_decoder *self)
{
    play_amr_handle_t amr = (play_amr_handle_t) self->userdata;
    return amr->has_post;
}

void play_amr_destroy_impl(struct play_decoder *self)
{
    play_amr_handle_t amr = (play_amr_handle_t) self->userdata;
    if (amr)
    {
        audio_free(amr);
    }

    AudioDecAmrClose();
    // AudioSendMsg(TYPE_AUDIO_AMR_DEC, MEDIA_MSGBOX_CMD_DECODE_CLOSE);
    g_amr_share_data.type = TYPE_DATA_MAX;

    RK_AUDIO_LOG_D("out");
}
#endif
