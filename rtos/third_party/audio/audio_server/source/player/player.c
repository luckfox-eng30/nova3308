/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "AudioConfig.h"

#ifdef AUDIO_PLUGIN_SOUNDTOUCH
#include "SoundTouch.h"
#define AUDIO_FORCE_16BITS      1
#else
#define AUDIO_FORCE_16BITS      0
#endif

#ifdef AUDIO_ENABLE_PLAYER
#define PLAYBACK_FRAMESIZE      (4096)  /* bytes */
#define DEVICE_IDLE_TIMEOUT     (2)     /* seconds */
#define SCALE_TABLE_SIZE        (4096)
#define MAX_HEADER_LEN          (1024)

play_decoder_t *g_default_decoder = NULL;
play_decoder_cfg_t *decoder_cfg = NULL;
play_preprocessor_cfg_t *processor_cfg = NULL;

struct player
{
    struct audio_player_queue *preprocess_queue;
    struct audio_player_queue *decode_queue;
    struct audio_player_queue *play_queue;

    struct audio_player_stream *preprocess_stream;
    struct audio_player_stream *decode_stream;

    audio_player_mutex_handle play_lock;
    audio_player_mutex_handle state_lock;
    audio_player_semaphore_handle pause_sem;
    audio_player_semaphore_handle stop_sem;

    audio_player_thread_handle preprocess_task;
    audio_player_thread_handle decode_task;
    audio_player_thread_handle play_task;

    player_state_t state;

    const char *tag;

    player_listen_cb listen;
    void *userdata;

    const char *name;
    playback_device_t device;
    play_preprocessor_t preprocessor;

    char *target;
    int need_free;
    int samplerate;
    int bits;
    int channels;
    uint32_t start_time;
    uint32_t cur_time;
    uint32_t total_time;
    uint32_t total_length;

#ifdef AUDIO_PLUGIN_SOUNDTOUCH
    SoundTouch *pST;
    int speed;
    int st_en;
#endif

#if AUDIO_FORCE_16BITS
    int post_bits;
#endif

    int resample_rate;
    int playback;
    int playback_start;
    int out_ch;
    int diff_out;
};

int player_init(void)
{
    play_decoder_t wav_decoder = DEFAULT_WAV_DECODER;
    player_register_decoder("wav", &wav_decoder);
    play_decoder_t pcm_decoder = DEFAULT_PCM_DECODER;
    player_register_decoder("pcm", &pcm_decoder);

    return 0;
}

int player_register_mp3dec(void)
{
#ifdef AUDIO_DECODER_MP3
    play_decoder_t mp3_decoder = DEFAULT_MP3_DECODER;
    return player_register_decoder("mp3", &mp3_decoder);
#else
    RK_AUDIO_LOG_E("Decoder MP3 is not enable");
    return RK_AUDIO_FAILURE;
#endif
}

int player_register_amrdec(void)
{
#ifdef AUDIO_DECODER_AMR
    play_decoder_t amr_decoder = DEFAULT_AMR_DECODER;
    return player_register_decoder("amr", &amr_decoder);
#else
    RK_AUDIO_LOG_E("Decoder AMR is not enable");
    return RK_AUDIO_FAILURE;
#endif
}

int player_register_apedec(void)
{
#ifdef AUDIO_DECODER_APE
    play_decoder_t ape_decoder = DEFAULT_APE_DECODER;
    return player_register_decoder("ape", &ape_decoder);
#else
    return 0;
#endif
}

int player_register_diffdec(void)
{
#ifdef AUDIO_DECODER_DIFF
    play_decoder_t diff_decoder = DEFAULT_DIFF_DECODER;
    return player_register_decoder("diff", &diff_decoder);
#else
    return 0;
#endif
}

int player_register_dsfdec(void)
{
#ifdef AUDIO_DECODER_DSF
    play_decoder_t dsf_decoder = DEFAULT_DSF_DECODER;
    return player_register_decoder("dsf", &dsf_decoder);
#else
    return 0;
#endif
}

int player_list_decoder(void)
{
    if (g_default_decoder == NULL)
    {
        RK_AUDIO_LOG_W("No decoder");
        return RK_AUDIO_FAILURE;
    }

    play_decoder_t *p = g_default_decoder;
    int i = 0;
    while (p)
    {
        RK_AUDIO_LOG_V("[Decoder %d] type:[%s]", i, p->type);
        p = p->next;
        i++;
    }

    return RK_AUDIO_SUCCESS;
}

int player_register_decoder(const char *type, play_decoder_t *decoder)
{
    play_decoder_t *dec;
    play_decoder_t *p = g_default_decoder;

    while (p)
    {
        if (!strcmp(type, p->type))
        {
            RK_AUDIO_LOG_V("Decoder [%s] already exist", type);
            return RK_AUDIO_SUCCESS;
        }
        p = p->next;
    }

    dec = audio_malloc(sizeof(play_decoder_t));
    if (!dec)
    {
        RK_AUDIO_LOG_E("malloc %d failed", sizeof(play_decoder_t));
        goto REG_EXIT;
    }
    dec->type = audio_malloc(strlen(type) + 1);
    if (!dec->type)
    {
        RK_AUDIO_LOG_E("malloc %d failed", strlen(type));
        audio_free(dec);
        goto REG_EXIT;
    }
    memset(dec->type, 0x0, strlen(type) + 1);
    memcpy(dec->type, type, strlen(type));
    dec->check = decoder->check;
    dec->init = decoder->init;
    dec->process = decoder->process;
    dec->get_post_state = decoder->get_post_state;
    dec->destroy = decoder->destroy;
    dec->support_seek = decoder->support_seek;
    dec->next = NULL;
    dec->userdata = NULL;
    if (g_default_decoder == NULL)
    {
        g_default_decoder = dec;
    }
    else
    {
        p = g_default_decoder;
        while (p->next)
            p = p->next;
        p->next = dec;
    }

    return RK_AUDIO_SUCCESS;
REG_EXIT:
    RK_AUDIO_LOG_V("Register [%s] failed", type);
    return RK_AUDIO_FAILURE;
}

int player_unregister_decoder(const char *type)
{
    play_decoder_t *target = NULL;
    play_decoder_t *p = g_default_decoder;

    if (!strcmp(type, p->type))
    {
        g_default_decoder = p->next;
        audio_free(p->type);
        audio_free(p);
        return RK_AUDIO_SUCCESS;
    }
    while (p->next)
    {
        if (!strcmp(type, p->next->type))
        {
            target = p->next;
            break;
        }
        p = p->next;
    }
    if (target)
    {
        p->next = target->next;
        audio_free(target->type);
        audio_free(target);
        return RK_AUDIO_SUCCESS;
    }
    else
    {
        RK_AUDIO_LOG_V("No this decoder [%s]", type);
        return RK_AUDIO_FAILURE;
    }
}

void *preprocess_run(void *data)
{
    player_handle_t player = (player_handle_t) data;
    media_sdk_msg_t msg;
    media_sdk_msg_t decode_msg;
    play_preprocessor_t preprocessor;
    int res;
    char *read_buf;
    size_t read_size = 0;
    size_t frame_size = 0;
    if (!processor_cfg)
    {
        processor_cfg = (play_preprocessor_cfg_t *)audio_malloc(sizeof(*processor_cfg));
        RK_AUDIO_LOG_D("malloc processor_cfg...");
    }

__PREPROCESS_ERROR:
    while (1)
    {
        if (audio_queue_receive(player->preprocess_queue, &msg) == -1)
        {
            RK_AUDIO_LOG_E("can't get preprocess msg");
            goto  __PREPROCESS_ERROR;
        }

        preprocessor = player->preprocessor;
        RK_AUDIO_LOG_D("preprocessor.type:%s", preprocessor.type);
        if (!preprocessor.type)
        {
            preprocessor.type = "unknown";
            RK_AUDIO_LOG_E("can't get preprocessor.type");
            goto  __PREPROCESS_ERROR;
        }

        processor_cfg->target = player->target;
        processor_cfg->tag = player->tag;
        processor_cfg->isOta = RK_AUDIO_FALSE;
        if (msg.type == CMD_PLAYER_SEEK)
            processor_cfg->seek_pos = msg.player.seek_pos;
        else
            processor_cfg->seek_pos = 0;

        if (player->start_time && preprocessor.seek == NULL)
            player->start_time = 0;

        res = preprocessor.init(&preprocessor, processor_cfg);
        if (res)
        {
            RK_AUDIO_LOG_E("init fail res:%d", res);
            player->state = PLAYER_STATE_ERROR;
            if (player->listen)
            {
                player->listen(player, PLAY_INFO_PREPROCESS, player->userdata);
            }
            goto  __PREPROCESS_ERROR;
        }

        player->total_length = processor_cfg->file_size;
        frame_size = processor_cfg->frame_size;
        read_buf = audio_malloc(frame_size);
        if (!read_buf)
        {
            RK_AUDIO_LOG_E("can't audio_malloc buffer %d", frame_size);
            player->state = PLAYER_STATE_ERROR;
            if (player->listen)
            {
                player->listen(player, PLAY_INFO_PREPROCESS, player->userdata);
            }
            goto PREPROCESS_EXIT;
        }

        decode_msg.type = msg.type;
        decode_msg.player.mode = msg.player.mode;
        decode_msg.player.target = msg.player.target;
        decode_msg.player.need_free = msg.player.need_free;
        decode_msg.player.end_session = msg.player.end_session;
        decode_msg.player.type = processor_cfg->type;

        RK_AUDIO_LOG_D("send msg to decode");
        if (msg.type == CMD_PLAYER_PLAY)
        {
            audio_stream_start(player->preprocess_stream);
            audio_queue_send(player->decode_queue, &decode_msg);
        }
        else if (msg.type == CMD_PLAYER_SEEK)
        {
            audio_stream_resume(player->preprocess_stream);
        }
        int timeout = 0;
        do
        {
retry:
            if (!audio_queue_is_empty(player->preprocess_queue))
            {
                if (audio_queue_receive(player->preprocess_queue, &msg) == -1)
                {
                    RK_AUDIO_LOG_E("can't get preprocess msg");
                    audio_stream_stop(player->preprocess_stream);
                    audio_free(read_buf);
                    preprocessor.destroy(&preprocessor);
                    goto  __PREPROCESS_ERROR;
                }
                if (msg.type == CMD_PLAYER_SEEK)
                {
                    processor_cfg->seek_pos = msg.player.seek_pos;
                    preprocessor.seek(&preprocessor, processor_cfg);
                    audio_stream_resume(player->preprocess_stream);
                }
            }
            read_size = preprocessor.read(&preprocessor, read_buf, frame_size);
            if (read_size == -101)
            {
                if ((player->state == PLAYER_STATE_IDLE) || (player->state == PLAYER_STATE_ERROR))
                {
                    RK_AUDIO_LOG_V("Player finish");
                    audio_stream_finish(player->preprocess_stream);
                    break;
                }
                timeout++;
                if (timeout > 20)
                {
                    //more than 10s exit
                    RK_AUDIO_LOG_E("Read timeout");
                    audio_stream_stop(player->preprocess_stream);
                    break;
                }
                RK_AUDIO_LOG_D("player state = %d", player->state);
                goto retry;
            }
            else if (read_size <= 0)
            {
                RK_AUDIO_LOG_V("read_size = %d, break", read_size);
                audio_stream_finish(player->preprocess_stream);
                break;
            }
            timeout = 0;
        }
        while (audio_stream_write(player->preprocess_stream, read_buf, read_size) != -1);
        audio_free(read_buf);
PREPROCESS_EXIT:
        preprocessor.destroy(&preprocessor);
        RK_AUDIO_LOG_V("out");
    }
}

#define MAX_RESAMPLE_INPUT_BYTES (128 * 2)

static SRCState *g_pSRC = NULL;
static int g_is_need_resample = 0;
static char *g_resample_buffer;
static short *rs_input_buffer;

int decoder_input(void *userdata, char *data, size_t data_len)
{
    player_handle_t player = (player_handle_t) userdata;
    return audio_stream_read(player->preprocess_stream, data, data_len);
}

void player_audio_resample_init(int samplerate, int resample_rate)
{
    int scale = resample_rate / samplerate + 1;
    int ret;

    rs_input_buffer = (short *)audio_malloc(MAX_RESAMPLE_INPUT_BYTES + 128 * sizeof(short));
    g_resample_buffer = (char *)audio_malloc(MAX_RESAMPLE_INPUT_BYTES * scale);
    g_pSRC = (SRCState *)audio_malloc(sizeof(SRCState));

    if ((rs_input_buffer == NULL) || (g_resample_buffer == NULL) || (g_pSRC == NULL))
    {
        RK_AUDIO_LOG_W("malloc failed, disable resample");
    }
    else
    {
        /* Only need to initialize the first 128 shorts */
        memset((char *)rs_input_buffer, 0, 128 * sizeof(short));
        ret = SRCInit(g_pSRC, samplerate, resample_rate);
        if (ret != 1)
        {
            RK_AUDIO_LOG_E("init failed %d->%d", samplerate, resample_rate);
            audio_free(g_pSRC);
            g_pSRC = NULL;
        }
    }
}

int player_audio_resample(player_handle_t player, char *data, size_t data_len)
{
    int resample_out_len = 0;
    int in_len;
    int ofs = 0;
    int ret = 0;

    if ((rs_input_buffer == NULL) || (g_resample_buffer == NULL) || (g_pSRC == NULL))
    {
        ret = audio_stream_write(player->decode_stream, data, data_len);
        return ret;
    }

    do
    {
        if (data_len > MAX_RESAMPLE_INPUT_BYTES)
            in_len = MAX_RESAMPLE_INPUT_BYTES;
        else
            in_len = data_len;
        memcpy((char *)&rs_input_buffer[128], data + ofs, in_len);
        resample_out_len = SRCFilter(g_pSRC, (short *)&rs_input_buffer[128], (short *)g_resample_buffer, in_len / 2);
        resample_out_len *= 2;
        ret |= audio_stream_write(player->decode_stream, g_resample_buffer, resample_out_len);
        data_len -= in_len;
        ofs += in_len;
    }
    while (data_len);

    return ret;
}

#if AUDIO_FORCE_16BITS
static int player_audio_xto16(int bits, char *data, size_t data_len)
{
    int start_bytes = 2;
    int in_step = 4;
    int out_len = data_len;

    if (bits == 16)
        return data_len;

    if (bits == 32)
    {
        in_step = 4;
        out_len = data_len / 2;
        start_bytes = 2;
    }
    else if (bits == 24)
    {
        in_step = 3;
        out_len = data_len * 2 / 3;
        start_bytes = 1;
    }

    for (int i = start_bytes, j = 0; i < data_len; i += in_step, j += 2)
    {
        data[j] = data[i];
        data[j + 1] = data[i + 1];
    }

    return out_len;
}
#endif

int decoder_output(void *userdata, char *data, size_t data_len)
{
    player_handle_t player = (player_handle_t) userdata;
    media_sdk_msg_t msg;
    int i = 0;
    int j = 0;
    float data_l;
    float data_r;
    float data_mix;
    int ret_len = data_len;
    int frame_bytes = player->bits / 8;
    int isMono = (player->channels == 1) ? 1 : 0;
    char *data_stero;
    int ret = 0;

    if (player->playback_start == 0)
    {
        msg.type = CMD_PLAYER_PLAY;
        msg.player.mode = PLAY_MODE_PROMPT;
        msg.player.target = NULL;
        msg.player.need_free = false;
        msg.player.end_session = false;
        audio_queue_send(player->play_queue, &msg);
        player->playback_start = 1;
        RK_AUDIO_LOG_V("Playback run");
    }

#if AUDIO_FORCE_16BITS
    data_len = player_audio_xto16(player->post_bits, data, data_len);
#endif
    /* if MONO, copy left channel data stream to right channels. */
    if (isMono)
    {
        j = data_len - frame_bytes;
        data_len *= 2;
        data_stero = audio_malloc(data_len);
        if (!player->diff_out)
        {
            for (i = data_len - 2 * frame_bytes; i >= 0; i = i - frame_bytes * 2)
            {
                memcpy(data_stero + i, data + j, frame_bytes);
                memcpy(data_stero + i + frame_bytes, data + j, frame_bytes);
                j -= frame_bytes;
            }
        }
        else
        {
            for (i = data_len - 2 * frame_bytes; i >= 0; i = i - frame_bytes * 2)
            {
                *(short *)(data_stero + i) = *(short *)(data + j);
                *(short *)(data_stero + i + frame_bytes) = -*(short *)(data + j);
                j -= frame_bytes;
            }
        }
    }
    else if (player->out_ch == 1)
    {
        for (i = 0; i < data_len / 2; i += 2)
        {
            data_l = (float)(((short *)data)[i]);
            data_r = (float)(((short *)data)[i + 1]);
            if (data_l > 0.0 && data_r > 0.0)
                data_mix = data_l + data_r - data_l * data_r / INT16_MAX;
            else if (data_l < 0.0 && data_r < 0.0)
                data_mix = data_l + data_r - data_l * data_r / INT16_MIN;
            else
                data_mix = data_l + data_r;
            if (!player->diff_out)
            {
                ((short *)data)[i + 1] = ((short *)data)[i] = (short)data_mix;
            }
            else
            {
                ((short *)data)[i] = (short)data_mix;
                ((short *)data)[i + 1] = -(short)data_mix;
            }
        }
        data_stero = data;
    }
    else
    {
        data_stero = data;
    }

#ifdef AUDIO_PLUGIN_SOUNDTOUCH
    if (player->st_en && player->speed)
    {
        int nSamples;
        uint32_t putLen = data_len / (player->channels * frame_bytes);
        SoundTouch_putSamples(player->pST, (SAMPLETYPE *)data_stero, putLen);
        do
        {
            nSamples = SoundTouch_receiveSamples(player->pST, (SAMPLETYPE *)data_stero, putLen);
            if (!nSamples)
                break;

            if (g_is_need_resample)
                ret = player_audio_resample(player, data_stero, nSamples * player->channels * frame_bytes);
            else
                ret = audio_stream_write(player->decode_stream, data_stero, nSamples * player->channels * frame_bytes);
            if (ret <= 0)
                break;
        }
        while (nSamples == putLen);
    }
    else
#endif
    {
        if (g_is_need_resample)
            ret = player_audio_resample(player, data_stero, data_len);
        else
            ret = audio_stream_write(player->decode_stream, data_stero, data_len);
    }

    if (isMono)
        audio_free(data_stero);

    if (ret < 0)
        return ret;

    return ret_len;
}

void player_audio_resample_deinit(void)
{
    RK_AUDIO_LOG_D("player_audio_resample_deinit");
    if (rs_input_buffer)
    {
        audio_free(rs_input_buffer);
        rs_input_buffer = NULL;
    }
    if (g_resample_buffer)
    {
        audio_free(g_resample_buffer);
        g_resample_buffer = NULL;
    }
    if (g_pSRC)
    {
        audio_free(g_pSRC);
        g_pSRC = NULL;
    }
}

uint32_t player_get_target_and_seek(char *file_name)
{
    RK_AUDIO_LOG_W("Do not use this API any more, call player_get_target(player, p_target)");
    return 0;
}

uint32_t player_get_target(player_handle_t self, char *file_name)
{
    if (self->target && file_name)
    {
        memcpy(file_name, self->target, strlen(self->target) + 1);
    }

    return (player_get_cur_time(self) / 1000);
}

void player_set_seek(long offset)
{
    RK_AUDIO_LOG_W("Do not use this API any more, Set cfg->start_time when call player_play");
}

int decoder_post(void *userdata, int samplerate, int bits, int channels)
{
    player_handle_t player = (player_handle_t)userdata;
    player->samplerate = samplerate;
#if AUDIO_FORCE_16BITS
    player->bits = 16;
    player->post_bits = bits;
#else
    player->bits = bits;
#endif
    player->channels = channels;

#ifdef AUDIO_PLUGIN_SOUNDTOUCH
    if (samplerate < 88200)
    {
        SoundTouch_setSampleRate(player->pST, samplerate);
        SoundTouch_setChannels(player->pST, 2/*channels*/);
        SoundTouch_setTempoChange(player->pST, player->speed);
        SoundTouch_setRateChange(player->pST, 0);
        SoundTouch_setPitchSemiTones(player->pST, 0);
        player->st_en = 1;
    }
    else
    {
        player->speed = 0;
        player->st_en = 0;
    }
#endif

    if (player->resample_rate && samplerate != player->resample_rate)
    {
        RK_AUDIO_LOG_V("%d != %d, need resample", samplerate, player->resample_rate);
        player_audio_resample_init(samplerate, player->resample_rate);
        player->samplerate = player->resample_rate;
        g_is_need_resample = 1;
    }
    else
    {
        g_is_need_resample = 0;
    }

    return RK_AUDIO_SUCCESS;
}

static void decoder_error(player_handle_t player)
{
    audio_stream_stop(player->preprocess_stream);
    audio_mutex_lock(player->state_lock);
    player->state = PLAYER_STATE_ERROR;
    if (player->listen)
        player->listen(player, PLAY_INFO_DECODE, player->userdata);
    audio_mutex_unlock(player->state_lock);
}

void *decoder_run(void *data)
{
    player_handle_t player = (player_handle_t) data;
    //play_decoder_cfg_t* decoder_cfg = (play_decoder_cfg_t*)audio_calloc(1,sizeof(*decoder_cfg));
    play_decoder_t decoder;
    char *header_buf;
    bool is_found_decoder = false;
    play_decoder_error_t decode_res;
    media_sdk_msg_t decode_msg;
    if (!decoder_cfg)
    {
        decoder_cfg = (play_decoder_cfg_t *)audio_calloc(1, sizeof(*decoder_cfg));
        decoder_cfg->input = decoder_input;
        decoder_cfg->output = decoder_output;
        decoder_cfg->post = decoder_post;
        decoder_cfg->userdata = player;
    }

    while (1)
    {
DECODER_WAIT:
        if (audio_queue_receive(player->decode_queue, &decode_msg) == -1)
        {
            RK_AUDIO_LOG_E("can't get msg");
            return NULL;
        }

        header_buf = audio_malloc(MAX_HEADER_LEN);
        if (header_buf == NULL)
        {
            RK_AUDIO_LOG_E("header buf %d malloc failed", MAX_HEADER_LEN);
            continue;
        }
        if (decoder_input(player, header_buf, MAX_HEADER_LEN) != MAX_HEADER_LEN)
        {
            RK_AUDIO_LOG_E("decoder input failed");
            decoder_error(player);
            audio_free(header_buf);
            continue;
        }

        int id3_len = check_ID3V2_tag(header_buf);
        if (id3_len)
        {
            int need_len = 0;
            id3_len += 10;
            if (id3_len >= MAX_HEADER_LEN)
            {
                int len;
                id3_len -= MAX_HEADER_LEN;
                while (id3_len)
                {
                    if (id3_len > MAX_HEADER_LEN)
                        len = MAX_HEADER_LEN;
                    else
                        len = id3_len;
                    if (decoder_input(player, header_buf, len) != len)
                    {
                        RK_AUDIO_LOG_E("decoder input failed");
                        decoder_error(player);
                        audio_free(header_buf);
                        goto DECODER_WAIT;
                    }
                    id3_len -= len;
                }
                need_len = MAX_HEADER_LEN;
            }
            else
            {
                memcpy(header_buf, header_buf + id3_len, MAX_HEADER_LEN - id3_len);
                need_len = id3_len;
            }
            if (need_len && decoder_input(player, header_buf, need_len) != need_len)
            {
                RK_AUDIO_LOG_E("decoder input failed");
                decoder_error(player);
                audio_free(header_buf);
                continue;
            }
        }

        play_decoder_t *p = g_default_decoder;
        is_found_decoder = false;
        /* Find decoder by header info */
        while (p)
        {
            if (p->check && p->check(header_buf, MAX_HEADER_LEN) == 0)
            {
                decoder = *p;
                is_found_decoder = true;
                RK_AUDIO_LOG_V("Found decoder %s.", p->type);
                break;
            }
            p = p->next;
        }

        audio_free(header_buf);

        /* Find decoder by file extension */
        if (is_found_decoder == false)
        {
            p = g_default_decoder;
            while (is_found_decoder == false && p)
            {
                if (!strcasecmp(decode_msg.player.type, p->type))
                {
                    decoder = *p;
                    is_found_decoder = true;
                    RK_AUDIO_LOG_V("Found decoder %s.", p->type);
                    break;
                }
                p = p->next;
            }
        }

        if (!is_found_decoder)
        {
            RK_AUDIO_LOG_E("can't found decoder");
            audio_stream_stop(player->preprocess_stream);
            audio_mutex_lock(player->state_lock);
            player->state = PLAYER_STATE_ERROR;
            if (player->listen)
                player->listen(player, PLAY_INFO_DECODE, player->userdata);
            audio_mutex_unlock(player->state_lock);
        }
        else
        {
            if (player->start_time && !decoder.support_seek)
                player->start_time = 0;
            RK_AUDIO_LOG_D("decode init.");
            player_preprocess_seek(player, 0);
#ifdef CONFIG_FWANALYSIS_SEGMENT
            FW_LoadSegment(decoder.segment, SEGMENT_OVERLAY_ALL);
#endif
            decoder_cfg->start_time = player->start_time;
            if (decoder.init(&decoder, decoder_cfg))
            {
                RK_AUDIO_LOG_E("decoder init fail");
                audio_stream_stop(player->preprocess_stream);
                audio_mutex_lock(player->state_lock);
                player->state = PLAYER_STATE_ERROR;
                if (player->listen)
                    player->listen(player, PLAY_INFO_DECODE, player->userdata);
                audio_mutex_unlock(player->state_lock);
                continue;
            }
            audio_stream_start(player->decode_stream);
            decode_res = decoder.process(&decoder);
            switch (decode_res)
            {

            case PLAY_DECODER_INPUT_ERROR:
            case PLAY_DECODER_OUTPUT_ERROR:
            case PLAY_DECODER_DECODE_ERROR:
                RK_AUDIO_LOG_E("decode res %d", decode_res);
#ifdef CONFIG_FWANALYSIS_SEGMENT
                FW_RemoveSegment(decoder.segment);
#endif
                audio_stream_stop(player->decode_stream);
                audio_stream_stop(player->preprocess_stream);
                if (!decoder.get_post_state(&decoder) || player->playback_start == 0)
                {
                    audio_mutex_lock(player->state_lock);
                    player->state = PLAYER_STATE_ERROR;
                    if (player->listen)
                    {
                        player->listen(player, PLAY_INFO_DECODE, player->userdata);
                    }
                    audio_mutex_unlock(player->state_lock);
                }
                break;

            default:
                RK_AUDIO_LOG_D("audio_stream_finish");
#ifdef CONFIG_FWANALYSIS_SEGMENT
                FW_RemoveSegment(decoder.segment);
#endif
                audio_stream_stop(player->preprocess_stream);
                audio_stream_finish(player->decode_stream);
                if (!decoder.get_post_state(&decoder) || player->playback_start == 0)
                {
                    audio_mutex_lock(player->state_lock);
                    player->state = PLAYER_STATE_ERROR;
                    if (player->listen)
                    {
                        player->listen(player, PLAY_INFO_IDLE, player->userdata);
                    }
                    audio_mutex_unlock(player->state_lock);
                }
                break;
            }
#if 0
            if (!decoder.get_post_state(&decoder))
            {
                audio_mutex_lock(player->state_lock);
                player->state = PLAYER_STATE_IDLE;
                if (player->listen)
                {
                    player->listen(player, PLAY_INFO_IDLE, player->userdata);
                }
                audio_mutex_unlock(player->state_lock);
            }
#endif
            RK_AUDIO_LOG_D("decoder process return value:%d", decode_res);
            decoder.destroy(&decoder);
            if (g_is_need_resample)
            {
                g_is_need_resample = 0;
                player_audio_resample_deinit();
            }
        }
    }
}

void *playback_run(void *data)
{
    player_handle_t player = (player_handle_t) data;
    playback_device_t device = player->device;
    playback_device_cfg_t device_cfg;
    media_sdk_msg_t msg;
    play_info_t info;
    char *read_buf = NULL;
    char *mute_buf = NULL;
    int read_size;
    size_t frame_size;
    static size_t oddframe = 0;
    int pcm_start_ret = 0;
    int byte2ms = 0;
    uint64_t total_byte;
    uint32_t idle_time_out;
    int32_t fade_in_samples = 0;
    int first_frame;
    while (1)
    {
        if (audio_queue_receive(player->play_queue, &msg) == -1)
        {
            RK_AUDIO_LOG_E("receive data failed");
            return NULL;
        }
        if (msg.player.end_session == true)
        {
            if (msg.player.need_free == true)
                audio_semaphore_give(player->stop_sem);
            continue;
        }
        device_cfg.samplerate = player->samplerate;
        device_cfg.bits = player->bits;
        device_cfg.channels = player->channels;
        device_cfg.card_name = player->name;
        device_cfg.frame_size = PLAYBACK_FRAMESIZE;
        if (!player->playback)
        {
            RK_AUDIO_LOG_D("No playback, break");
            player->state = PLAYER_STATE_RUNNING;
            continue;
        }
        RK_AUDIO_LOG_V("start");
        if (device.open(&device, &device_cfg))
        {
            audio_stream_stop(player->preprocess_stream);
            audio_stream_stop(player->decode_stream);
            audio_mutex_lock(player->state_lock);
            player->state = PLAYER_STATE_ERROR;
            if (player->listen)
            {
                player->listen(player, PLAY_INFO_STOP, player->userdata);
            }
            audio_mutex_unlock(player->state_lock);
            RK_AUDIO_LOG_E("device open fail");
            continue;
        }
        else
        {
            frame_size = device_cfg.frame_size;
            pcm_start_ret = device.start(&device);
            if (pcm_start_ret != 0)
            {
                RK_AUDIO_LOG_E("pcm device start fail.");
                audio_stream_stop(player->preprocess_stream);
                audio_stream_stop(player->decode_stream);
                audio_mutex_lock(player->state_lock);
                player->state = PLAYER_STATE_ERROR;
                if (player->listen)
                {
                    player->listen(player, PLAY_INFO_STOP, player->userdata);
                }
                audio_mutex_unlock(player->state_lock);
                continue;
            }
            read_buf = audio_malloc(frame_size * 2);
            if (!read_buf)
            {
                RK_AUDIO_LOG_E("create read buf fail");
                audio_stream_stop(player->preprocess_stream);
                audio_stream_stop(player->decode_stream);
                audio_mutex_lock(player->state_lock);
                player->state = PLAYER_STATE_ERROR;
                if (player->listen)
                {
                    player->listen(player, PLAY_INFO_STOP, player->userdata);
                }
                audio_mutex_unlock(player->state_lock);
                device.stop(&device);
                device.close(&device);
                continue;
            }
PLAYBACK_START:
            memset(read_buf, 0, frame_size * 2);
            /* byte2ms = channels * (bits >> 3) * (rate / 1000), channels will always be 2 */
            byte2ms = 2 * (device_cfg.bits >> 3) * player->samplerate / 1000;
            /* If decoder seek not accurate, this may not accurate */
            if (player->start_time)
            {
                total_byte = player->start_time * 1000 * byte2ms;
                player->cur_time = player->start_time * 1000;
            }
            else
            {
                total_byte = 0;
                player->cur_time = 0;
            }
            idle_time_out =
                device_cfg.samplerate * DEVICE_IDLE_TIMEOUT *
                (device_cfg.bits >> 3) * device_cfg.channels / device_cfg.frame_size;
            player->state = PLAYER_STATE_RUNNING;
            first_frame = 1;
            info = PLAY_INFO_IDLE;
            while (1)
            {
                //OS_LOG_D(player,"playback_run:read frame_size:%d",frame_size);
                memset(read_buf + oddframe, 0x0, frame_size);
                read_size = audio_stream_read(player->decode_stream, read_buf + oddframe, frame_size);
                if (read_size == -1)
                {
                    RK_AUDIO_LOG_W("Force stop");
                    info = PLAY_INFO_STOP;
                    /* Fade out */
                    short *data = (short *)(read_buf + oddframe);
                    short *last = (short *)((oddframe == 0) ? (read_buf + frame_size) : read_buf);
                    short last_data_l = *(last + frame_size / 2 - 2);
                    short last_data_r = *(last + frame_size / 2 - 1);
                    fade_in_samples = SCALE_TABLE_SIZE;
                    while (fade_in_samples > 0)
                    {
                        uint32_t len =
                            (frame_size / 2 / sizeof(short)) < SCALE_TABLE_SIZE ?
                            (frame_size / 2 / sizeof(short)) : SCALE_TABLE_SIZE;
                        for (int i = 0; i < len; i++)
                        {
                            float s = (float)(fade_in_samples - i) / SCALE_TABLE_SIZE;
                            s *= s;
                            data[i * 2] = (short)((float)last_data_l * s);
                            data[i * 2 + 1] = (short)((float)last_data_r * s);
                        }
                        device.write(&device, (char *)data, frame_size);
                        memset((char *)data, 0, frame_size);
                        fade_in_samples -= len;
                    }
                    break;
                }
                if (read_size == 0)
                {
                    RK_AUDIO_LOG_V("Finish");
                    audio_stream_stop(player->decode_stream);
                    break;
                }
                if (first_frame == 1)
                {
                    /* Fade in init */
                    first_frame = 0;
                    fade_in_samples = SCALE_TABLE_SIZE;
                }
                if (fade_in_samples > 0)
                {
                    /* Fade in */
                    short *data = (short *)(read_buf + oddframe);
                    uint32_t len =
                        (frame_size / 2 / sizeof(short)) < fade_in_samples ?
                        (frame_size / 2 / sizeof(short)) : fade_in_samples;
                    for (int i = 0; i < len; i++)
                    {
                        float s = (float)(i + SCALE_TABLE_SIZE - fade_in_samples + 1) / SCALE_TABLE_SIZE;
                        s *= s;
                        data[i * 2] *= s;
                        data[i * 2 + 1] *= s;
                    }
                    fade_in_samples -= len;
                }
                switch (player->state)
                {
                case PLAYER_STATE_PAUSED:
                {
                    RK_AUDIO_LOG_D("PLAYER_STATE_PAUSED.");
                    RK_AUDIO_LOG_D("play pause");
                    mute_buf = (oddframe == 0) ? (read_buf + frame_size) : read_buf;
                    memset(mute_buf, 0x0, frame_size);
                    while (audio_semaphore_try_take(player->pause_sem))
                        device.write(&device, mute_buf, frame_size);
                    if (player->state == PLAYER_STATE_STOP)
                    {
                        RK_AUDIO_LOG_D("play resume->stop");
                        info = PLAY_INFO_STOP;
                        goto PLAYBACK_STOP;
                    }
                    RK_AUDIO_LOG_D("play resume");
                    /* fall through */
                }
                case PLAYER_STATE_RUNNING:
                {
                    total_byte += read_size;
                    player->cur_time = total_byte / byte2ms;
                    device.write(&device, read_buf + oddframe, read_size);
                    break;
                }
                default:
                {
                    break;
                }
                }
                if (read_size < frame_size)
                {
                    RK_AUDIO_LOG_D("underrun.");
                    break;
                }
                oddframe = (oddframe == 0) ? frame_size : 0;
            }
PLAYBACK_STOP:
            audio_mutex_lock(player->state_lock);
            player->state = PLAYER_STATE_IDLE;
            if (player->listen)
            {
                player->listen(player, info, player->userdata);
            }
            audio_mutex_unlock(player->state_lock);
#ifndef OS_IS_FREERTOS
            mute_buf = (oddframe == 0) ? (read_buf + frame_size) : read_buf;
            memset(mute_buf, 0x0, frame_size);
            while (idle_time_out--)
            {
                device.write(&device, mute_buf, frame_size);
                if (audio_queue_is_empty(player->play_queue) != true)
                {
                    audio_queue_receive(player->play_queue, &msg);
                    if (msg.player.end_session == true)
                    {
                        if (msg.player.need_free == true)
                            audio_semaphore_give(player->stop_sem);
                        idle_time_out = 0;
                        break;
                    }
                    else
                    {
                        goto PLAYBACK_START;
                    }
                }
            }
            device.stop(&device);
            device.close(&device);
#endif
            if (read_buf)
            {
                RK_AUDIO_LOG_D("free read_buf");
                audio_free(read_buf);
                read_buf = NULL;
            }
            RK_AUDIO_LOG_V("out");
        }
    }
}

player_handle_t player_create(player_cfg_t *cfg)
{
    player_handle_t player = (player_handle_t) audio_calloc(1, sizeof(*player));
    uint32_t preprocess_stack_size;
    uint32_t decoder_stack_size;
    uint32_t playback_stack_size;

    RK_AUDIO_LOG_D("in");
    if (player)
    {
        memset((void *)player, 0x0, sizeof(*player));
        player->preprocess_queue = audio_queue_create(1, sizeof(media_sdk_msg_t));
        player->decode_queue = audio_queue_create(1, sizeof(media_sdk_msg_t));
        player->play_queue = audio_queue_create(1, sizeof(media_sdk_msg_t));
        player->preprocess_stream = audio_stream_create(cfg->preprocess_buf_size);
        player->decode_stream = audio_stream_create(cfg->decode_buf_size);
        player->state_lock = audio_mutex_create();
        player->play_lock = audio_mutex_create();
        player->pause_sem = audio_semaphore_create();
        player->stop_sem = audio_semaphore_create();
        player->tag = cfg->tag;
        player->listen = cfg->listen;
        player->userdata = cfg->userdata;
        player->name = cfg->name;
        player->device = cfg->device;
        player->resample_rate = cfg->resample_rate ? ((cfg->resample_rate == -1) ? 0 : cfg->resample_rate) : 48000;
        player->diff_out = cfg->diff_out == 1 ? 1 : 0;
        player->out_ch = cfg->out_ch == 1 ? 1 : (player->diff_out == 1 ? 1 : 2);
        player->state = PLAYER_STATE_IDLE;

#ifdef AUDIO_PLUGIN_SOUNDTOUCH
        player->speed = 0;
        player->pST = audio_malloc(sizeof(SoundTouch));
        SoundTouch_init(player->pST);
        SoundTouch_setPitchSemiTones(player->pST, 0);
        SoundTouch_setRateChange(player->pST, 0);
        SoundTouch_setTempoChange(player->pST, 0);
#endif

        preprocess_stack_size = cfg->preprocess_stack_size ? cfg->preprocess_stack_size : 4096;
        decoder_stack_size = cfg->decoder_stack_size ? cfg->decoder_stack_size : 1024 * 12;
        playback_stack_size = cfg->playback_stack_size ? cfg->playback_stack_size : 2048;
        audio_thread_cfg_t c =
        {
            .run = (void *)preprocess_run,
            .args = player
        };
        player->preprocess_task = audio_thread_create("preprocess_task",
                                                      preprocess_stack_size,
                                                      PLAYER_TASK_PRIORITY, &c);
        c.run = (void *)decoder_run;
        c.args = player;
        player->decode_task = audio_thread_create("decode_task",
                                                  decoder_stack_size,
                                                  PLAYER_TASK_PRIORITY, &c);
        c.run = (void *)playback_run;
        c.args = player;
        player->play_task = audio_thread_create("play_task",
                                                playback_stack_size,
                                                PLAYER_TASK_PRIORITY, &c);
#ifdef OS_IS_FREERTOS
        struct audio_menuconfig cfg;
        rkos_audio_get_config(&cfg, AUDIO_FLAG_WRONLY);
        playback_set_volume(cfg.play.vol);
#endif
        RK_AUDIO_LOG_V("Success %s 0x%lx 0x%lx 0x%lx",
                       player->name,
                       preprocess_stack_size,
                       decoder_stack_size,
                       playback_stack_size);
    }
    else
    {
        RK_AUDIO_LOG_E("Failure");
    }

    return player;
}

void player_set_speed(player_handle_t self, int speed)
{
#ifdef AUDIO_PLUGIN_SOUNDTOUCH
    if (!self->st_en)
        return;

    if (speed < -50)
        speed = -50;
    else if (speed > 100)
        speed = 100;

    self->speed = speed;

    SoundTouch_setTempoChange(self->pST, self->speed);
#else
    RK_AUDIO_LOG_E("No support set speed");
#endif
}

int player_play(player_handle_t self, play_cfg_t *cfg)
{
    player_handle_t player = self;
    media_sdk_msg_t msg ;
    int time_out = 2000;

    audio_mutex_lock(player->play_lock);
    if (player->state == PLAYER_STATE_RUNNING)
        player_stop(player);

    player->preprocessor = cfg->preprocessor;
    player->samplerate = cfg->samplerate;
    player->bits = cfg->bits;
    player->channels = cfg->channels;
    player->start_time = cfg->start_time;
    player->cur_time = 0;
    player->total_time = 0;
    player->playback = cfg->info_only ? 0 : 1;
    player->playback_start = 0;
    msg.type = CMD_PLAYER_PLAY;
    msg.player.mode = PLAY_MODE_PROMPT;
    msg.player.need_free = false;
    msg.player.end_session = false;

    if (player->need_free && player->target != NULL)
    {
        audio_free(player->target);
        player->target = NULL;
    }

    player->need_free = cfg->need_free;

    if (cfg->need_free)
    {
        player->target = audio_malloc(strlen(cfg->target) + 1);
        if (player->target == NULL)
        {
            RK_AUDIO_LOG_E("no mem!");
            return RK_AUDIO_FAILURE;
        }
        memcpy(player->target, cfg->target, strlen(cfg->target) + 1);
    }
    else
    {
        player->target = cfg->target;
    }

    RK_AUDIO_LOG_V("Target [%s], start %ld s", player->target, player->start_time);

    audio_queue_send(player->preprocess_queue, &msg);
    while ((self->state != PLAYER_STATE_RUNNING) && (self->state != PLAYER_STATE_ERROR))
    {
        audio_sleep(10);
        time_out--;
        if (!time_out)
            break;
    }
    if (self->state == PLAYER_STATE_RUNNING && self->playback == 0)
    {
        audio_stream_resume(player->preprocess_stream);
        player_stop(player);
        goto PLAYER_PLAY_OUT;
    }
PLAYER_PLAY_OUT:
    audio_mutex_unlock(player->play_lock);

    return RK_AUDIO_SUCCESS;
}

int player_preprocess_seek(player_handle_t self, uint32_t pos)
{
    media_sdk_msg_t msg;

    if (!self || !self->preprocess_queue)
        return RK_AUDIO_FAILURE;

    msg.type = CMD_PLAYER_SEEK;
    msg.player.seek_pos = pos;
    if (audio_queue_send(self->preprocess_queue, &msg) != RK_AUDIO_SUCCESS)
        return RK_AUDIO_FAILURE;
    audio_stream_reset(self->preprocess_stream);

    return RK_AUDIO_SUCCESS;
}

player_state_t player_get_state(player_handle_t self)
{
    player_state_t state;
    audio_mutex_lock(self->state_lock);
    state = self->state;
    audio_mutex_unlock(self->state_lock);
    return state;
}

int player_get_cur_time(player_handle_t self)
{
    uint32_t time = self->cur_time;
    /* After seek, cur time may be not accurate */
    if (self->total_time && self->total_time < time)
        return self->total_time;
    else
        return time;
}

uint32_t player_get_file_length(player_handle_t self)
{
    return self->total_length;
}

void player_set_total_time(player_handle_t self, uint32_t time)
{
    self->total_time = time;
}

uint32_t player_get_total_time(player_handle_t self)
{
    return self->total_time;
}

int player_stop(player_handle_t self)
{
    int result;
    audio_mutex_lock(self->state_lock);
    self->cur_time = 0;
    if (self->state)
    {
        audio_stream_stop(self->preprocess_stream);
        RK_AUDIO_LOG_D("audio_stream_stop preprocess_stream");
        audio_stream_stop(self->decode_stream);
        if (self->state == PLAYER_STATE_PAUSED)
        {
            self->state = PLAYER_STATE_STOP;
            audio_semaphore_give(self->pause_sem);
        }
        audio_mutex_unlock(self->state_lock);
        while (self->playback && (self->state != PLAYER_STATE_IDLE) && (self->state != PLAYER_STATE_ERROR))
        {
            audio_sleep(10);
        }
        self->state = PLAYER_STATE_IDLE;
        result = 0;
        RK_AUDIO_LOG_V("stop player,pause/running state");
    }
    else
    {
        self->state = PLAYER_STATE_IDLE;
        audio_mutex_unlock(self->state_lock);
        RK_AUDIO_LOG_V("stop player,idle state");
        result = 0;
    }

    return result;
}

int player_device_stop(player_handle_t self, int wait)
{
    audio_mutex_lock(self->play_lock);
    if (self->state == PLAYER_STATE_RUNNING)
        player_stop(self);
    media_sdk_msg_t msg;
    msg.player.end_session = true;
    if (wait)
        msg.player.need_free = true;
    else
        msg.player.need_free = false;
    audio_queue_send(self->play_queue, &msg);
    if (wait)
        audio_semaphore_take(self->stop_sem);
    audio_mutex_unlock(self->play_lock);

    return RK_AUDIO_SUCCESS;
}

int player_pause(player_handle_t self)
{
    audio_mutex_lock(self->state_lock);
    if (self->state == PLAYER_STATE_RUNNING)
    {
        self->state = PLAYER_STATE_PAUSED;
    }
    audio_mutex_unlock(self->state_lock);

    return RK_AUDIO_SUCCESS;
}

int player_resume(player_handle_t self)
{
    audio_mutex_lock(self->state_lock);
    if (self->state == PLAYER_STATE_PAUSED)
    {
        self->state = PLAYER_STATE_RUNNING;
        audio_semaphore_give(self->pause_sem);
    }
    audio_mutex_unlock(self->state_lock);

    return RK_AUDIO_SUCCESS;
}

int player_wait_idle(player_handle_t self)
{
    if (self->listen)
    {
        audio_mutex_lock(self->state_lock);
        self->listen(self, self->state, self->userdata);
        if (self->state == PLAYER_STATE_IDLE)
        {
            RK_AUDIO_LOG_D("idle.....");
            audio_mutex_unlock(self->state_lock);
            return PLAYER_STATE_IDLE;
        }
        audio_mutex_unlock(self->state_lock);
    }
    return RK_AUDIO_SUCCESS;
}

int player_close(player_handle_t self)
{
    playback_device_t device = self->device;

    if (self->state)
        player_stop(self);
    device.stop(&device);
    device.close(&device);

    return RK_AUDIO_SUCCESS;
}

void player_destroy(player_handle_t self)
{
    player_handle_t player = self;
    playback_device_t device = player->device;
    RK_AUDIO_LOG_D("player_destory in");
    if (player)
    {
        if (player->state)
            player_stop(player);
        device.stop(&device);
        device.close(&device);
        audio_queue_destroy(player->preprocess_queue);
        audio_queue_destroy(player->decode_queue);
        audio_queue_destroy(player->play_queue);
        audio_stream_destroy(player->preprocess_stream);
        audio_stream_destroy(player->decode_stream);
        audio_mutex_destroy(player->state_lock);
        audio_mutex_destroy(player->play_lock);
        audio_semaphore_destroy(player->pause_sem);
        audio_semaphore_destroy(player->stop_sem);
        audio_thread_exit(player->preprocess_task);
        audio_thread_exit(player->decode_task);
        audio_thread_exit(player->play_task);
        if (player->need_free && player->target != NULL)
        {
            audio_free(player->target);
            player->target = NULL;
        }
    }
    if (decoder_cfg)
    {
        audio_free(decoder_cfg);
        decoder_cfg = NULL;
    }
    if (processor_cfg)
    {
        audio_free(processor_cfg);
        processor_cfg = NULL;
    }
    RK_AUDIO_LOG_D("player_destory player free.");
    audio_free(player);
    player = NULL;
}

void player_deinit()
{
    if (g_default_decoder)
    {
        RK_AUDIO_LOG_D("player deinit.");
        play_decoder_t *p = g_default_decoder;
        int i = 0;
        while (p)
        {
            g_default_decoder = p->next;
            RK_AUDIO_LOG_D("free [Decoder %d] type:[%s]", i, p->type);
            audio_free(p->type);
            audio_free(p);
            p = g_default_decoder;
            i++;
        }
        g_default_decoder = NULL;
    }
}
#endif
