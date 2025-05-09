/* layer3.c */


#include "mp3_enc_types.h"

/* Scalefactor bands. */
static int sfBandIndex[4][3][23] =
{
    { /* MPEG-2.5 11.025 kHz */
        {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576},
        /* MPEG-2.5 12 kHz */
        {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576},
        /* MPEG-2.5 8 kHz */
        {0, 12, 24, 36, 48, 60, 72, 88, 108, 132, 160, 192, 232, 280, 336, 400, 476, 566, 568, 570, 572, 574, 576}
    },
    {
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0},
        {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
    },
    { /* Table B.2.b: 22.05 kHz */
        {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576},
        /* Table B.2.c: 24 kHz */
        {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 114, 136, 162, 194, 232, 278, 332, 394, 464, 540, 576},
        /* Table B.2.a: 16 kHz */
        {0, 6, 12, 18, 24, 30, 36, 44, 54, 66, 80, 96, 116, 140, 168, 200, 238, 284, 336, 396, 464, 522, 576}
    },
    { /* Table B.8.b: 44.1 kHz */
        {0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 52, 62, 74, 90, 110, 134, 162, 196, 238, 288, 342, 418, 576},
        /* Table B.8.c: 48 kHz */
        {0, 4, 8, 12, 16, 20, 24, 30, 36, 42, 50, 60, 72, 88, 106, 128, 156, 190, 230, 276, 330, 384, 576},
        /* Table B.8.a: 32 kHz */
        {0, 4, 8, 12, 16, 20, 24, 30, 36, 44, 54, 66, 82, 102, 126, 156, 194, 240, 296, 364, 448, 550, 576}
    }
};

static int find_samplerate_index(config_t *config, long freq)
{
    long sr[4][3] =  {{11025, 12000,  8000},   /* mpeg 2.5 */
        {    0,     0,     0},   /* reserved */
        {22050, 24000, 16000},   /* mpeg 2 */
        {44100, 48000, 32000}
    };  /* mpeg 1 */
    int i, j;

    for (j = 0; j < 4; j++)
        for (i = 0; i < 3; i++)
            if ((freq == sr[j][i]) && (j != 1))
            {
                config->mpeg.type = j;
                return i;
            }

    return 0;
}

static int find_bitrate_index(config_t *config, int bitr)
{
    long br[2][15] =
    {
        {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160}, /* mpeg 2/2.5 */
        {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320}
    };  /* mpeg 1 */
    int i;

    for (i = 1; i < 15; i++)
        if (bitr <= br[config->mpeg.type & 1][i]) return i;

    return 0;
}

static int set_cutoff(config_t *config)
{
    int cutoff_tab[3][2][15] =
    {
        { /* 44.1k, 22.05k, 11.025k */
            {100, 104, 131, 157, 183, 209, 261, 313, 365, 418, 418, 418, 418, 418, 418}, /* stereo */
            {183, 209, 261, 313, 365, 418, 418, 418, 418, 418, 418, 418, 418, 418, 418} /* mono */
        },
        { /* 48k, 24k, 12k */
            {100, 104, 131, 157, 183, 209, 261, 313, 384, 384, 384, 384, 384, 384, 384}, /* stereo */
            {183, 209, 261, 313, 365, 384, 384, 384, 384, 384, 384, 384, 384, 384, 384} /* mono */
        },
        { /* 32k, 16k, 8k */
            {100, 104, 131, 157, 183, 209, 261, 313, 365, 418, 522, 576, 576, 576, 576}, /* stereo */
            {183, 209, 261, 313, 365, 418, 522, 576, 576, 576, 576, 576, 576, 576, 576} /* mono */
        }
    };

    return cutoff_tab[config->mpeg.samplerate_index]
           [config->mpeg.mode == MODE_MONO]
           [config->mpeg.bitrate_index];
}

static mp3_enc *Mp3EncodeHeaderInit(config_t *config)
{
    mp3_enc *enc = malloc(sizeof(mp3_enc));
    if (!enc)
        return NULL;

    memset(enc, 0, sizeof(mp3_enc));
    if (config->mpeg.type == MPEG1)
    {
        config->mpeg.granules = 2;
        config->mpeg.samples_per_frame = samp_per_frame;
        config->mpeg.resv_limit = ((1 << 9) - 1) << 3;
        enc->info.sideinfo_len = (config->mpeg.channels == 1) ? 168 : 288;
    }
    else /* mpeg 2/2.5 */
    {
        config->mpeg.granules = 1;
        config->mpeg.samples_per_frame = samp_per_frame2;
        config->mpeg.resv_limit = ((1 << 8) - 1) << 3;
        enc->info.sideinfo_len = (config->mpeg.channels == 1) ? 104 : 168;
    }
    enc->scalefac_band_long = sfBandIndex[config->mpeg.type][config->mpeg.samplerate_index];

    {
        /* find number of whole bytes per frame and the remainder */
        long x = config->mpeg.samples_per_frame * config->mpeg.bitr * (1000 / 8);
        enc->info.bytes_per_frame = x / config->mpeg.samplerate;
        enc->info.remainder  = x % config->mpeg.samplerate;
    }
    enc->info.lag = 0;
    memcpy(&enc->config, config, sizeof(config_t));
    open_bit_stream(enc);

    return enc;
}

mp3_enc *Mp3EncodeVariableInit(int samplerate, int channel, int  Bitrate)
{
    config_t config;

    config.mpeg.samplerate = samplerate; //sampling rate. 8000
    config.mpeg.channels = channel;//1
    config.mpeg.bitr = Bitrate; //32
    if (config.mpeg.samplerate <= 11025)
    {
        config.mpeg.type = MPEG2_5;
        if (config.mpeg.bitr > 160)
        {
            config.mpeg.bitr = 160;
        }
    }
    else if (config.mpeg.samplerate <= 22050)
    {
        config.mpeg.type = MPEG2;
        if (config.mpeg.bitr > 160)
        {
            config.mpeg.bitr = 160;
        }
    }
    else
    {
        config.mpeg.type = MPEG1;
        if (config.mpeg.bitr > 320)
        {
            config.mpeg.bitr = 320;
        }
    }
    if (config.mpeg.channels == 1)
    {
        config.mpeg.mode = MODE_MONO;
    }
    else
    {
        config.mpeg.mode = MODE_STEREO;
        // config.mpeg.mode = MODE_DUAL_CHANNEL;
        // config.mpeg.mode = MODE_MS_STEREO;
    }
    config.mpeg.layr = LAYER_3;
    config.mpeg.psyc = 0;
    config.mpeg.emph = 0;
    config.mpeg.crc  = 0;
    config.mpeg.ext  = 0;
    config.mpeg.mode_ext  = 0;
    config.mpeg.copyright = 0;
    config.mpeg.original  = 1;
    config.mpeg.granules = 1; //与MP3格式有关(标准)，会被重新赋值

    config.mpeg.samplerate_index = find_samplerate_index(&config, config.mpeg.samplerate);
    config.mpeg.bitrate_index    = find_bitrate_index(&config, config.mpeg.bitr);
    config.mpeg.cutoff = set_cutoff(&config);
    config.in_buf = malloc(2 * 1152 * sizeof(short));

    mp3_enc *mp3 = Mp3EncodeHeaderInit(&config);

    mp3->frame_size = mp3->config.mpeg.samples_per_frame * mp3->config.mpeg.channels;

    return mp3;
}

/*
 * L3_compress:
 * ------------
 */
long L3_compress(mp3_enc *mp3, int len, unsigned char **ppOutBuf)
{
    int           ch;
    int           i;
    int           gr;
    int           write_bytes;
    unsigned long *buffer_window[2];
    buffer_window[0] = buffer_window[1] = (unsigned long *)mp3->config.in_buf;
    /* sort out padding */
    mp3->config.mpeg.padding = (mp3->info.lag += mp3->info.remainder) >= mp3->config.mpeg.samplerate;
    if (mp3->config.mpeg.padding)
        mp3->info.lag -= mp3->config.mpeg.samplerate;
    mp3->config.mpeg.bits_per_frame = 8 * (mp3->info.bytes_per_frame + mp3->config.mpeg.padding);

    /* bits per channel per granule */
    mp3->info.mean_bits = (mp3->config.mpeg.bits_per_frame - mp3->info.sideinfo_len) >>
                          (mp3->config.mpeg.granules + mp3->config.mpeg.channels - 2);

    /* polyphase filtering */
    for (gr = 0; gr < mp3->config.mpeg.granules; gr++)
        for (ch = 0; ch < mp3->config.mpeg.channels; ch++)
            for (i = 0; i < 18; i++)
                L3_window_filter_subband(mp3, &buffer_window[ch], &mp3->l3_sb_sample[ch][gr + 1][i][0], ch);

#if 1

    L3_mdct_sub(mp3);
    L3_iteration_loop(mp3);
#else
    L3_mdct_sub(mp3, mp3->l3_sb_sample, mdct_freq);

    /* bit and noise allocation */
    L3_iteration_loop(mdct_freq, &side_info, l3_enc, mp3_enc->info.mean_bits);
#endif

    /* write the frame to the bitstream */
    write_bytes = L3_format_bitstream(mp3, ppOutBuf);

    return write_bytes;
}

void Mp3EncodeDeinit(mp3_enc *mp3)
{
    if (mp3)
    {
        close_bit_stream(mp3);
        free(mp3->config.in_buf);
        free(mp3);
    }
}

