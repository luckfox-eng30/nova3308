/*
 * SPDX-License-Identifier: BSD-3-Clause
 *
 * Copyright (c) 2019 Fuzhou Rockchip Electronics Co., Ltd
 */

#include "amr_dec_hal.h"

#include "interf_dec.h"
#include "dec_if.h"

/*
 * #define    AMR_DEC_ERROR(format,...)    RK_AUDIO_LOG_E("[AmrDec] Error: "format, ##__VA_ARGS__)
 * #define    AMR_DEC_INFO(format,...)     RK_AUDIO_LOG_D("[AmrDec] Info: "format, ##__VA_ARGS__)
 */
#define    AMR_DEC_ERROR(format,...)
#define    AMR_DEC_INFO(format,...)

static struct amr_dec g_amr_dec;

int32_t AudioDecAmrOpen(uint32_t A2B_ShareDatAddr)
{
    struct audio_server_data_share *pDecDat = (struct audio_server_data_share *)A2B_ShareDatAddr;

    memset(&g_amr_dec, 0, sizeof(struct amr_dec));

    g_amr_dec.is_wb = ((int)pDecDat->dat[0] == AMR_WB ? 1 : 0);
    g_amr_dec.is_mono = ((int)pDecDat->dat[1] == 1 ? 1 : 0);

    g_amr_dec.ab_share_dat = pDecDat;
    if (g_amr_dec.is_wb)
        g_amr_dec._decoder = D_IF_init();
    else
        g_amr_dec._decoder = Decoder_Interface_init();
    if (NULL == g_amr_dec._decoder)
    {
        AMR_DEC_ERROR("AudioDecAMROpen Faild!!!");
        return -1;
    }

    pDecDat->dat[0] = (uint32_t)&g_amr_dec;
    pDecDat->dat[1] = 0;

    return 0;
}

int32_t AudioDecAmrProcess(uint32_t decode_dat)
{
    short buffer[SAMPLE_PER_FRAME_WB] = {0};
    uint8_t *ptr = NULL;
    int samples;
    int i;

    samples = g_amr_dec.is_wb ? SAMPLE_PER_FRAME_WB : SAMPLE_PER_FRAME;

    if (g_amr_dec.is_wb)
        D_IF_decode(g_amr_dec._decoder, g_amr_dec._buffer_in, buffer, 0);
    else
        Decoder_Interface_Decode(g_amr_dec._decoder, g_amr_dec._buffer_in, buffer, 0);

    ptr = (uint8_t *)g_amr_dec._buffer_out;
    for (i = 0; i < samples; i++)
    {
        *ptr++ = (buffer[i] >> 0) & 0xff;
        *ptr++ = (buffer[i] >> 8) & 0xff;
    }

    return 0;
}


int32_t AudioDecAmrClose(void)
{
    AMR_DEC_INFO("amr decode close\n");

    if (g_amr_dec._decoder)
    {
        if (g_amr_dec.is_wb)
            D_IF_exit(g_amr_dec._decoder);
        else
            Decoder_Interface_exit(g_amr_dec._decoder);
        g_amr_dec._decoder = NULL;
    }
    return 0;
}
